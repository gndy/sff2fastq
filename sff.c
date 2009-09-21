/* I N C L U D E S ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include "sff.h"

/* F U N C T I O N S *********************************************************/
void 
read_sff_common_header(FILE *fp, sff_common_header *h) {
    char *flow;
    char *key;
    int header_size;

    fread(&(h->magic)          , sizeof(uint32_t), 1, fp);
    fread(&(h->version)        , sizeof(char)    , 4, fp);
    fread(&(h->index_offset)   , sizeof(uint64_t), 1, fp);
    fread(&(h->index_len)      , sizeof(uint32_t), 1, fp);
    fread(&(h->nreads)         , sizeof(uint32_t), 1, fp);
    fread(&(h->header_len)     , sizeof(uint16_t), 1, fp);
    fread(&(h->key_len)        , sizeof(uint16_t), 1, fp);
    fread(&(h->flow_len)       , sizeof(uint16_t), 1, fp);
    fread(&(h->flowgram_format), sizeof(uint8_t) , 1, fp);

    /* sff files are in big endian notation so adjust appropriately */
    h->magic        = htobe32(h->magic);
    h->index_offset = htobe64(h->index_offset);
    h->index_len    = htobe32(h->index_len);
    h->nreads       = htobe32(h->nreads);
    h->header_len   = htobe16(h->header_len);
    h->key_len      = htobe16(h->key_len);
    h->flow_len     = htobe16(h->flow_len);

    /* finally appropriately allocate and read the flow and key strings */
    flow = (char *) malloc( h->flow_len * sizeof(char) );
    if (!flow) {
        printf("Out of memory! Could not allocate header flow string!\n");
        exit(1);
    }

    key  = (char *) malloc( h->key_len  * sizeof(char) );
    if (!key) {
        printf("Out of memory! Could not allocate header key string!\n");
        exit(1);
    }

    fread(flow, sizeof(char), h->flow_len, fp);
    h->flow = flow;

    fread(key , sizeof(char), h->key_len , fp);
    h->key = key;

    /* the common header section should be a multiple of 8-bytes 
       if the header is not, it is zero-byte padded to make it so */

    header_size = sizeof(h->magic)
                  + sizeof(*(h->version))*4
                  + sizeof(h->index_offset)
                  + sizeof(h->index_len)
                  + sizeof(h->nreads)
                  + sizeof(h->header_len)
                  + sizeof(h->key_len)
                  + sizeof(h->flow_len)
                  + sizeof(h->flowgram_format)
                  + (sizeof(char) * h->flow_len)
                  + (sizeof(char) * h->key_len);

    printf("The header size is currently %d bytes\n", header_size);

    if ( !(header_size % PADDING_SIZE == 0) ) {
        read_padding(fp, header_size);
    }
}

void
read_padding(FILE *fp, int header_size) {
    int remainder = PADDING_SIZE - (header_size % PADDING_SIZE);
    printf("There is a %d byte padding in the common header\n", remainder);
    uint8_t padding[remainder];
    fread(padding, sizeof(uint8_t), remainder, fp);
    printf("padding: %s\n", padding);
}

void
free_sff_common_header(sff_common_header *h) {
    free(h->flow);
    free(h->key);
}

void 
verify_sff_common_header(char *prg_name, 
                         char *prg_version, 
                         sff_common_header *h) {

    /* ensure that the magic file type is valid */
    if (h->magic != SFF_MAGIC) {
        fprintf(stderr, "The SFF header has magic value '%d' \n", h->magic);
        fprintf(stderr, 
                "[err] %s (version %s) %s : '%d' \n", 
                prg_name, 
                prg_version, 
                "only knows how to deal an SFF magic value of type",
                SFF_MAGIC);
        free_sff_common_header(h);
        exit(2);
    }

    /* ensure that the version header is valid */
    if ( memcmp(h->version, SFF_VERSION, SFF_VERSION_LENGTH) ) {
        fprintf(stderr, "The SFF file has header version: ");
        int i;
        char *sff_header_version = h->version;
        for (i=0; i < SFF_VERSION_LENGTH; i++) {
            printf("0x%02x ", sff_header_version[i]);
        }
        printf("\n");
        fprintf(stderr, 
                "[err] %s (version %s) %s : ", 
                prg_name, 
                prg_version, 
                "only knows how to deal an SFF header version: ");
        char valid_header_version[SFF_VERSION_LENGTH] = SFF_VERSION;
        for (i=0; i < SFF_VERSION_LENGTH; i++) {
            printf("0x%02x ", valid_header_version[i]);
        }
        free_sff_common_header(h);
        exit(2);
    }
}

void
read_sff_read_header(FILE *fp, sff_read_header *rh) {
    char *name;
    int header_size;

    fread(&(rh->header_len)        , sizeof(uint16_t), 1, fp);
    fread(&(rh->name_len)          , sizeof(uint16_t), 1, fp);
    fread(&(rh->nbases)            , sizeof(uint32_t), 1, fp);
    fread(&(rh->clip_qual_left)    , sizeof(uint16_t), 1, fp);
    fread(&(rh->clip_qual_right)   , sizeof(uint16_t), 1, fp);
    fread(&(rh->clip_adapter_left) , sizeof(uint16_t), 1, fp);
    fread(&(rh->clip_adapter_right), sizeof(uint16_t), 1, fp);

    /* sff files are in big endian notation so adjust appropriately */
    rh->header_len         = htobe16(rh->header_len);
    rh->name_len           = htobe16(rh->name_len);
    rh->nbases             = htobe32(rh->nbases);
    rh->clip_qual_left     = htobe16(rh->clip_qual_left);
    rh->clip_qual_right    = htobe16(rh->clip_qual_right);
    rh->clip_adapter_left  = htobe16(rh->clip_adapter_left);
    rh->clip_adapter_right = htobe16(rh->clip_adapter_right);

    /* finally appropriately allocate and read the read_name string */
    name = (char *) malloc( rh->name_len * sizeof(char) );
    if (!name) {
        printf("Out of memory! Could not allocate header flow string!\n");
        exit(1);
    }

    fread(name, sizeof(char), rh->name_len, fp);
    rh->name = name;

    /* the section should be a multiple of 8-bytes, if not,
       it is zero-byte padded to make it so */

    header_size = sizeof(rh->header_len)
                  + sizeof(rh->name_len)
                  + sizeof(rh->nbases)
                  + sizeof(rh->clip_qual_left)
                  + sizeof(rh->clip_qual_right)
                  + sizeof(rh->clip_adapter_left)
                  + sizeof(rh->clip_adapter_right)
                  + (sizeof(char) * rh->name_len);

    printf("The read header size is currently %d bytes\n", header_size);

    if ( !(header_size % PADDING_SIZE == 0) ) {
        read_padding(fp, header_size);
    }
}

void
free_sff_read_header(sff_read_header *h) {
    free(h->name);
}

void
read_sff_read_data(FILE *fp, 
                   sff_read_data *rd, 
                   uint16_t nflows, 
                   uint32_t nbases) {
    uint16_t *flowgram;
    uint8_t *flow_index;
    uint8_t *quality;
    char *bases;
    int data_size;
    int i;

    /* allocate the appropriate arrays */
    flowgram   = (uint16_t *) malloc( nflows * sizeof(uint16_t) );
    if (!flowgram) {
        printf("Out of memory! Could not allocate for a read flowgram!\n");
        exit(1);
    }

    flow_index = (uint8_t *) malloc( nbases * sizeof(uint8_t)  );
    if (!flow_index) {
        printf("Out of memory! Could not allocate for a read flow index!\n");
        exit(1);
    }

    quality = (uint8_t *) malloc( nbases * sizeof(uint8_t)  );
    if (!quality) {
        printf("Out of memory! Could not allocate for a read quality string!\n");
        exit(1);
    }

    bases = (char * ) malloc( nbases * sizeof(char) );
    if (!bases) {
        printf("Out of memory! Could not allocate for a read base string!\n");
        exit(1);
    }

    /* read from the sff file and assign to sff_read_data */
    fread(flowgram, sizeof(uint16_t), (size_t) nflows, fp);

    /* sff files are in big endian notation so adjust appropriately */
    for (i = 0; i < nflows; i++) {
        flowgram[i] = htobe16(flowgram[i]);
    }
    rd->flowgram = flowgram;

    fread(flow_index, sizeof(uint8_t), (size_t) nbases, fp);
    rd->flow_index = flow_index;

    fread(bases, sizeof(char), (size_t) nbases, fp);
    rd->bases = bases;

    fread(quality, sizeof(uint8_t), (size_t) nbases, fp);
    rd->quality = quality;

    /* the section should be a multiple of 8-bytes, if not,
       it is zero-byte padded to make it so */

    data_size = (sizeof(uint16_t) * nflows)    // flowgram size
                + (sizeof(uint8_t) * nbases)   // flow_index size
                + (sizeof(char) * nbases)      // bases size
                + (sizeof(uint8_t) * nbases);  // quality size
                  

    printf("The read data size is currently %d bytes\n", data_size);

    if ( !(data_size % PADDING_SIZE == 0) ) {
        read_padding(fp, data_size);
    }
}

void
free_sff_read_data(sff_read_data *d) {
    free(d->flowgram);
    free(d->flow_index);
    free(d->bases);
    free(d->quality);
}
