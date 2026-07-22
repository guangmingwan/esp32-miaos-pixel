#include "decloader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern GmuDecoder *gmu_register_decoder(void);
extern GmuDecoder *gmu_register_vorbis_decoder(void);
extern GmuDecoder *gmu_register_flac_decoder(void);

static DecoderChain *root;
static char extensions[256];

static DecoderChain *new_node(void) {
    DecoderChain *node = calloc(1, sizeof(*node));
    return node;
}

int decloader_load_builtin_decoders(void) {
    GmuDecoder *decoders[] = {
        gmu_register_decoder(),
        gmu_register_vorbis_decoder(),
        gmu_register_flac_decoder(),
    };
    DecoderChain *tail = NULL;
    extensions[0] = '\0';
    root = NULL;

    for (size_t i = 0; i < sizeof(decoders) / sizeof(decoders[0]); ++i) {
        DecoderChain *node = new_node();
        if (node == NULL) return 0;
        node->gd = decoders[i];
        if (tail == NULL) root = node;
        else tail->next = node;
        tail = node;
        snprintf(extensions + strlen(extensions), sizeof(extensions) - strlen(extensions),
                 "%s;", node->gd->get_file_extensions());
    }
    if (tail != NULL) tail->next = new_node();
    return root != NULL;
}

GmuDecoder *decloader_get_decoder_for_extension(const char *file_extension) {
    if (root == NULL || file_extension == NULL) return NULL;
    for (DecoderChain *node = root; node != NULL && node->gd != NULL; node = node->next) {
        if (strstr(node->gd->get_file_extensions(), file_extension) != NULL)
            return node->gd;
    }
    return NULL;
}

GmuDecoder *decloader_get_decoder_for_mime_type(const char *mime_type) {
    (void)mime_type;
    return NULL;
}

GmuDecoder *decloader_get_decoder_for_data_chunk(const char *data, size_t size) {
    (void)data;
    (void)size;
    return NULL;
}

char *decloader_get_all_extensions(void) { return extensions; }

GmuDecoder *decloader_decoder_list_get_next_decoder(int getfirst) {
    static DecoderChain *cursor;
    cursor = getfirst ? root : (cursor == NULL ? NULL : cursor->next);
    return cursor == NULL ? NULL : cursor->gd;
}

void decloader_free(void) {
    while (root != NULL) {
        DecoderChain *next = root->next;
        free(root);
        root = next;
    }
    extensions[0] = '\0';
}

GmuDecoder *decloader_load_decoder(const char *so_file) { (void)so_file; return NULL; }
int decloader_load_all(const char *directory) { (void)directory; return 0; }
