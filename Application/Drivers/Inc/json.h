/*
 * json.h
 *
 *  Created on: Mar 25, 2020
 *      Author: ivan
 */

#ifndef DRIVERS_INC_JSON_H_
#define DRIVERS_INC_JSON_H_

struct json_values_t {
    char key[50];
    char value[50];
};

void json_process(char *, struct json_values_t *);
#endif /* DRIVERS_INC_JSON_H_ */
