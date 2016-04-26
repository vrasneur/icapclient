icapclient
===

A Python module for creating ICAP clients.
The module API is somewhat inspired by the [httplib](https://docs.python.org/2/library/httplib.html) python module.

This module is written in pure C, and uses the C-ICAP library to
handle the ICAP protocol.

What is an ICAP client?
---

ICAP is a protocol defined in [RFC3507](http://tools.ietf.org/html/rfc3507). [Errata](http://www.measurement-factory.com/std/icap/) are available for the ICAP specification and there is a draft RFC about [ICAP extensions](https://tools.ietf.org/html/draft-stecher-icap-subid-00).

It is a client-server protocol that is primarily used to do virus scanning and content rewriting. E.g. an ICAP client sends a potentially infected file to the ICAP server. This server does some analysis and sends back information about the file to the client, such as the detected virus name. Or a proxy can send content to an ICAP server that will modify the content and send it back to the proxy.

ICAP is implemented by many antivirus gateways or web proxies. You have a list of implementations in the [Wikipedia article about ICAP](http://en.wikipedia.org/wiki/Internet_Content_Adaptation_Protocol) or at the [ICAP-forum Products page](http://www.icap-forum.org/icap?do=products).

Requirements
---

* Python 2.6 or 2.7
* the [C-ICAP](http://c-icap.sourceforge.net) library, tested on
  versions 0.1.6, 0.3.4 and 0.3.5
* GCC or clang

Installation
---

1. Install all the required development packages.

   E.g. on a Debian system, as root :

   ```bash
   # install the Python development stuff
   apt-get install python-dev
   # install the C-ICAP API
   apt-get install libicapapi-dev
   ```

2. Build the library, as a normal user

   ```bash
   cd /path/to/icapclient
   python setup.py build
   ```

3. Install the library, as root

   ```bash
   python setup.py install
   ```

Example
---

To send files and get the ICAP server response

```python
# import the icap client library
>>> import icapclient
# for pretty printing
>>> from pprint import pprint
# create an ICAP connection, default port is 1344
>>> conn = icapclient.ICAPConnection('192.168.1.5')
# check that the ICAP server recognizes the EICAR virus test file
# send a REQMOD request, default url is '/' and default service is 'avscan'
# specify another service explicitly if you are using "eset" for example
>>> conn.request('REQMOD', '/home/vincent/malwares/eicar.txt', service="avscan")
# get the server response
>>> resp = conn.getresponse()
# get the ICAP response status code
>>> resp.icap_status
200
# get the ICAP response reason string
>>> resp.icap_reason
'OK'
# print the ICAP response headers
# they are a list of (name, value) tuples
>>> pprint(resp.icap_headers)
[('Server', 'C-ICAP/0.3.4'),
 ('Connection', 'keep-alive'),
 ('ISTag', 'CI0001-xhZPmAmHArrMLzamxkH5CwAA'),
 ('X-Infection-Found', 'Type=0; Resolution=2; Threat=Eicar-Test-Signature;'),
 ('Encapsulated', 'res-hdr=0, res-body=108')]
# get a specific header value
# here, check if a virus has been found
# Note that "x-infection-found" is a draft and not all antivirues implement it.
# For example, Eset NOT 32 does not.
# Sometimes you should look inside the incapsulated HTTP-response.
>>> resp.get_icap_header('x-infection-found')
'Type=0; Resolution=2; Threat=Eicar-Test-Signature;'
# get the first line of the encapsulated HTTP request
>>> resp.http_req_line
'POST / HTTP/1.1'
# get the first line of the encapsulated HTTP response
>>> resp.http_resp_line
'HTTP/1.0 403 Forbidden'
# okay, test files are great, but try with a real PDF exploit
# send the REQMOD request and reuse the socket
>>> conn.request('REQMOD', '/home/vincent/malwares/exploit.pdf')
# get the ICAP response
>>> resp = conn.getresponse()
# the response is OK
>>> resp.icap_status
200
>>> resp.icap_reason
'OK'
# pretty-print the ICAP response headers
>>> pprint(resp.icap_headers)
[('Server', 'C-ICAP/0.3.4'),
 ('Connection', 'keep-alive'),
 ('ISTag', 'CI0001-xhZPmAmHArrMLzamxkH5CwAA'),
 ('X-Infection-Found', 'Type=0; Resolution=2; Threat=Exploit.PDF-28560;'),
 ('Encapsulated', 'res-hdr=0, res-body=108')]
# does the request contain a PDF exploit? yes
>>> resp.get_icap_header('x-infection-found')
'Type=0; Resolution=2; Threat=Exploit.PDF-28560;'
>>> resp.http_resp_line
'HTTP/1.0 403 Forbidden'
# try a REQMOD request with a file with no malware in it
>>> conn.request('REQMOD', '/home/vincent/files/normal.txt')
>>> resp = conn.getresponse()
>>> resp.icap_status
200
>>> resp.icap_reason
'OK'
>>> pprint(resp.icap_headers)
[('Server', 'C-ICAP/0.3.4'),
 ('Connection', 'keep-alive'),
 ('ISTag', 'CI0001-xhZPmAmHArrMLzamxkH5CwAA'),
 ('Encapsulated', 'req-hdr=0, req-body=124')]
# no virus or malware found
>>> resp.get_icap_header('x-infection-found') is None
True
# close the ICAP connection
>>> conn.close()
```

To enable the verbose mode

```python
>>> import icapclient
>>> icapclient.set_debug_stdout(True)
>>> icapclient.set_debug_level(10)
```
