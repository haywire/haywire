typedef char* (*http_request_callback)();
extern http_request_callback http_req_callback;

int hw_http_open(char *ipaddress, int port);
void hw_http_register_request_callback(http_request_callback callback);