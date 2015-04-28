//! Implemented by file object returned by http_request::run methods. Allows you to retrieve various additional information returned by the server. \n
//! Warning: reply status may change when seeking on the file object since seek operations often require a new HTTP request to be fired.
class NOVTABLE http_reply : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(http_reply, service_base)
public:
	//! Retrieves the status line, eg. "200 OK".
	virtual void get_status(pfc::string_base & out) = 0;
	//! Retrieves a HTTP header value, eg. "content-type". Note that get_http_header("content-type", out) is equivalent to get_content_type(out). If there are multiple matching header entries, value of the first one will be returned.
	virtual bool get_http_header(const char * name, pfc::string_base & out) = 0;
	//! Retrieves a HTTP header value, eg. "content-type". If there are multiple matching header entries, this will return all their values, delimited by \r\n.
	virtual bool get_http_header_multi(const char * name, pfc::string_base & out) = 0;
};

class NOVTABLE http_request : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(http_request, service_base)
public:
	//! Adds a HTTP request header line.
	//! @param line Request to be added, without trailing \r\n.
	virtual void add_header(const char * line) = 0;
	//! Runs the request on the specified URL. Throws an exception on failure (connection error, invalid response from the server, reply code other than 2XX), returns a file::ptr interface to the stream on success.
	virtual file::ptr run(const char * url, abort_callback & abort) = 0;
	//! Runs the request on the specified URL. Throws an exception on failure but returns normally if the HTTP server returned a valid response other than 2XX, so the caller can still parse the received data stream if the server has returned an error.
	virtual file::ptr run_ex(const char * url, abort_callback & abort) = 0;

	void add_header(const char * name, const char * value) {
		add_header(PFC_string_formatter() << name << ": " << value);
	}
};

class NOVTABLE http_request_post : public http_request {
	FB2K_MAKE_SERVICE_INTERFACE(http_request_post, http_request);
public:
	//! Adds a HTTP POST field.
	//! @param name Field name.
	//! @param fileName File name to be included in the POST request; leave empty ("") not to send a file name.
	//! @param contentType Content type of the entry; leave empty ("") not to send content type.
	virtual void add_post_data(const char * name, const void * data, t_size dataSize, const char * fileName, const char * contentType) = 0;

	void add_post_data(const char * name, const char * value) { add_post_data(name, value, strlen(value), "", ""); }
};

class NOVTABLE http_client : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(http_client)
public:
	//! Creates a HTTP request object.
	//! @param type Request type. Currently supported: "GET" and "POST". Throws pfc::exception_not_implemented for unsupported values.
	virtual http_request::ptr create_request(const char * type) = 0;
};
