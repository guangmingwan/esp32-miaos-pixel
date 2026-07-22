#include "feloader.h"

#include <stdlib.h>

extern GmuFrontend *gmu_register_frontend(void);

static FrontendChainElement *root;

static FrontendChainElement *new_node(void) {
    return calloc(1, sizeof(FrontendChainElement));
}

int feloader_load_builtin_frontends(void) {
    root = new_node();
    if (root == NULL) return 0;
    root->gf = gmu_register_frontend();
    if (root->gf->frontend_init != NULL && root->gf->frontend_init() == 0) return 0;
    root->next = new_node();
    return 1;
}

GmuFrontend *feloader_frontend_list_get_next_frontend(int getfirst) {
    static FrontendChainElement *cursor;
    cursor = getfirst ? root : (cursor == NULL ? NULL : cursor->next);
    return cursor == NULL ? NULL : cursor->gf;
}

void feloader_free(void) {
    if (root != NULL && root->gf != NULL && root->gf->frontend_shutdown != NULL)
        root->gf->frontend_shutdown();
    while (root != NULL) {
        FrontendChainElement *next = root->next;
        free(root);
        root = next;
    }
}

int feloader_load_all(char *directory) { (void)directory; return feloader_load_builtin_frontends(); }
int feloader_load_single_frontend(char *so_file) { (void)so_file; return feloader_load_builtin_frontends(); }
GmuFrontend *feloader_load_frontend(char *so_file) { (void)so_file; return gmu_register_frontend(); }
int feloader_unload_frontend(GmuFrontend *frontend) { if (frontend && frontend->frontend_shutdown) frontend->frontend_shutdown(); return 1; }
