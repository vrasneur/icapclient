icapclient3
===========

A Python3 module for creating ICAP clients. This is a fork of
https://github.com/vrasneur/icapclient with Python3.X support. The
module API is somewhat inspired by the `httplib`_ python module.

This module is written in pure C, and uses the C-ICAP library to handle
the ICAP protocol.

What is an ICAP client?
-----------------------

ICAP is a protocol defined in `RFC3507`_. `Errata`_ are available for
the ICAP specification and there is a draft RFC about `ICAP
extensions`_.

It is a client-server protocol that is primarily used to do virus
scanning and content rewriting. E.g. an ICAP client sends a potentially
infected file to the ICAP server. This server does some analysis and
sends back information about the file to the client, such as the
detected virus name. Or a proxy can send content to an ICAP server that
will modify the content and send it back to the proxy.

ICAP is implemented by many antivirus gateways or web proxies. You have
a list of implementations in the `Wikipedia article about ICAP`_ or at
the `ICAP-forum Products page`_.

Requirements
------------

-  Python 3.X
-  the `C-ICAP`_ library, tested on versions 0.1.6, 0.3.4 and 0.3.5
-  GCC or clang

Installation
------------

1. Install all the required development packages.

   E.g. on a Debian system, as root :

   .. code:: bash

       # install the Python development stuff
       apt-get install python-dev
       # install the C-ICAP API
       apt-get install libicapapi-dev

2. Build the library, as a normal user

   .. code:: bash

       cd /path/to/icapclient
       python setup.py build

3. Install the library, as root

   .. code:: bash

       python setup.py install

Example
-------

To send files and get the ICAP server response

\```python # import the icap client library >>> import icapclient # for
pretty printing >>> from pprint import pprint # create an ICAP
connection, default port is 1344 >>> conn =
icapclient.ICAPConnection(‘192.168.1.5’) # check that the ICAP server
recognizes the EICAR virus test file # send a REQMOD request, default
url is ‘/’ and default service is ‘avscan’ # specify another service
explicitly if you are using “eset” for example >>>
conn.request(‘REQMOD’, ‘/home/vincent/malwares/eicar.txt’,
service=“avscan”) # get the server response >>> resp =
conn.getresponse() # get the ICAP response status code >>>
resp.icap_status 200 # get the ICAP response reason string >>>
resp.icap_reason ‘OK’ # print the ICAP response headers # they are a
list of (name, value) tuples >>> pprint(resp.icap_headers) [(‘Server’,
‘C-ICAP/0.3.4’), (‘Connection’, ‘keep-alive’), (‘ISTag’,
‘CI0001-xhZPmAmHArrMLzamxkH5CwAA’), (‘X-Infection-Found’, ‘Type=0;
Resolution=2; Threat=Eicar-Test-Signature;’), (’Encaps

.. _httplib: https://docs.python.org/2/library/httplib.html
.. _RFC3507: http://tools.ietf.org/html/rfc3507
.. _Errata: http://www.measurement-factory.com/std/icap/
.. _ICAP extensions: https://tools.ietf.org/html/draft-stecher-icap-subid-00
.. _Wikipedia article about ICAP: http://en.wikipedia.org/wiki/Internet_Content_Adaptation_Protocol
.. _ICAP-forum Products page: http://www.icap-forum.org/icap?do=products
.. _C-ICAP: http://c-icap.sourceforge.net
