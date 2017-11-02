#!/usr/bin/env python
# -*- mode: python; coding: utf-8 -*-

from distutils.core import setup, Extension
from distutils.spawn import find_executable

import os
from subprocess import check_output
import sys

if sys.version_info < (3,):
    sys.stderr.write("Only Python3 or higher is supported.")
    sys.exit(1)

api_config = 'c-icap-libicapapi-config'

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

if not find_executable(api_config):
    raise OSError('Cannot find the "%s" command' % api_config)

extra_compile_args = ['-std=gnu99', '-Wextra']
extra_compile_args.extend(check_output([api_config, '--cflags']).decode('utf-8').split())
extra_link_args = check_output([api_config, '--libs']).decode('utf-8').split()

ext = Extension(name='icapclient', sources=['icapclient.c', 'ICAPConnection.c', 'ICAPResponse.c', 'cicap_compat.c'],
                extra_compile_args=extra_compile_args,
                extra_link_args=extra_link_args)

name_str = 'icapclient3'
version_str = '1.0.6'
url_str = 'https://github.com/fim/%s' % name_str
tarball_str = '%s/tarball/%s' % (url_str, version_str)

setup(name=name_str,
      version=version_str,
      description='Python3 module for creating ICAP clients',
      long_description=read("README.rst"),
      author='Serafeim Mellos',
      author_email='fim@mellos.io',
      url=url_str,
      download_url=tarball_str,
      keywords=['icap', 'antivirus'],
      ext_modules=[ext])
