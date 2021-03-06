#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

#include <errno.h>

struct ips_record {
	uint32_t off;
	uint16_t len;
	int is_rle;
	uint8_t data[UINT16_MAX];
};

static uint32_t ips_read_u24(FILE* fp) {
	uint8_t buf[4] = {0};
	fread(buf, sizeof(uint8_t), 3, fp);
	return ntohl(*(uint32_t*)buf) >> 8;
}

static void ips_write_u24(FILE* fp, uint32_t i) {
	uint8_t *buf = (uint8_t*)&i;
	i = htonl(i << 8);

	fwrite(buf, sizeof(uint8_t), 3, fp);
}

static uint16_t ips_read_u16(FILE* fp) {
	uint16_t i = 0;
	fread(&i, sizeof(uint16_t), 1, fp);
	return ntohs(i);
}

static void ips_write_u16(FILE* fp, uint16_t i) {
	i = htons(i);
	fwrite(&i, sizeof(uint16_t), 1, fp);
}

static void write_record(FILE* fp, struct ips_record *rec) {
	static char header[5] = {'P', 'A', 'T', 'C', 'H'};

	if (header[0]) {
		fwrite(header, sizeof(char), 5, fp);
		header[0] = 0; /* write header only once */
	}
	ips_write_u24(fp, rec->off);

	if (rec->is_rle) {
		ips_write_u16(fp, 0);
		ips_write_u16(fp, rec->len);
		fputc(rec->data[0], fp);
	} else {
		ips_write_u16(fp, rec->len);
		fwrite(rec->data, sizeof(uint8_t), rec->len, fp);
	}
}

static int read_record(FILE* fp, struct ips_record *rec) {
	uint8_t rleval;

	rec->off = ips_read_u24(fp);

	if (rec->off == *(uint32_t*)"EOF" || feof(fp))
		return 0;

	rec->len = ips_read_u16(fp);
	rec->is_rle = rec->len == 0;

	if (rec->is_rle) {
		rec->len = ips_read_u16(fp);
		rleval   = fgetc(fp);

		rec->data[0] = rleval;
	} else {
		fread(rec->data, sizeof(uint8_t), rec->len, fp);
	}

	return 1;
}

static void behead(struct ips_record *in, struct ips_record *out) {
	size_t skip = 0;
	ssize_t new_off = (ssize_t)in->off - 0x200;

	out->off = new_off;
	out->len = in->len;
	out->is_rle = in->is_rle;

	if (new_off < 0) { /* record starts in the header */
		out->off = 0;
		out->len = in->len + new_off;
		skip = -new_off;

		if (out->len > in->len) {
			out->len = 0;
			return;
		}
	}

	if (out->len) {
		if (out->is_rle)
			out->data[0] = in->data[0];
		else
			memcpy(out->data, &in->data[skip], out->len);

	}
}

#if 0
static void randomize_record_data(struct ips_record *r) {
	size_t i;

	for (i = 0; i < sizeof(r->data); i += sizeof(int32_t)) {
		*(int32_t*)&r->data[i] = rand();
	}
}

static void run_tests() {
	/* Usecase 1: Input record doesn't touch the header. */
	{
		struct ips_record in = { 0x200, 16 };
		struct ips_record out = { 0 };
		randomize_record_data(&in);

		behead(&in, &out);

		assert(out.off == (in.off - 0x200));
		assert(out.len == in.len);
		assert(out.is_rle == in.is_rle);
		assert(memcmp(in.data, out.data, in.is_rle ? 1 : in.len) == 0);
	}

	/* Usecase 2: Input record modifies only the header. */
	{
		struct ips_record in = { 0x20, 1, 0 };
		struct ips_record out = { 0 };
		randomize_record_data(&in);

		behead(&in, &out);

		assert(out.len == 0);
		assert(out.is_rle == in.is_rle);
	}

	/* Usecase 3: Input record modifies both the header and the body. */
	{
		struct ips_record in = { 0x200-8, 16, 0 };
		struct ips_record out = { 0 };
		randomize_record_data(&in);

		behead(&in, &out);

		assert(out.off == 0);
		assert(out.len == 8);
		assert(out.is_rle == in.is_rle);
		assert(memcmp(&in.data[8], out.data, out.len) == 0);
	}
}
#endif

int main (int argc, char* argv[]) {
	FILE *ifp = NULL;
	FILE *ofp = NULL;
	struct ips_record in = {0};
	struct ips_record out = {0};

	int chunkno = 0;

	if (argc < 3) {
		printf("usage: %s <input.ips> <output.ips>\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	ifp = fopen(argv[1], "rb");
	ofp = fopen(argv[2], "wb");

	{
		char tmp[6] = {0};
		fread(tmp, sizeof(char), 5, ifp);

		if (strcmp("PATCH", tmp) != 0) {
			puts("invalid input file.\n");
			exit(EXIT_FAILURE);
		}
	}


	printf("%-5s | %-15s | %-15s | %-6s\n", "CHUNK", "BEFORE", "AFTER ", "RLE");

	while (read_record(ifp, &in)) {
		behead(&in, &out);

		if (out.len)
			write_record(ofp, &out);

		printf("%5i | %06x - %06x | %06x - %06x | %s\n",
				chunkno,
				in.off, (in.len ? (in.off + in.len-1) : in.off),
				out.off, (out.len ? (out.off + out.len-1) : out.off),
				(in.is_rle) ? "Y" : "N");
		chunkno++;
	}

	fwrite("EOF", sizeof(char), 3, ofp);

	if (!feof(ifp)) {
		printf("truncate ignored");
	}

	fclose(ofp);
	fclose(ifp);

	return 0;
}

