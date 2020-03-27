/*
 * http.h
 *
 *  Created on: Mar 26, 2020
 *      Author: ivan
 */

#ifndef DRIVERS_INC_HTTP_H_
#define DRIVERS_INC_HTTP_H_

void web_server_subscribe();
void http_set_message(char *);
void http_set_measurement_auth(char *);
void http_set_measurement_url(char *);

#endif /* DRIVERS_INC_HTTP_H_ */
