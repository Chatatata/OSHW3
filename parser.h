//
//  parser.h
//  libexamwriters
//
//  Created by Buğra Ekuklu on 06.04.2016.
//  Copyright © 2016 The Digital Warehouse. All rights reserved.
//

#ifndef parser_h
#define parser_h

#include <stdio.h>
#include "dispatch.h"

dispatch_t **readf(int num_workers, const char *file_name);

#endif /* parser_h */
