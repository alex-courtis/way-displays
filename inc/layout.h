#ifndef LAYOUT_H
#define LAYOUT_H

#include "types.h"

void desire_ltr(struct OutputManager *output_manager);

void pend_desired(struct OutputManager *output_manager);

void apply_desired(struct OutputManager *output_manager);

void print_desired(struct OutputManager *output_manager);

#endif // LAYOUT_H

