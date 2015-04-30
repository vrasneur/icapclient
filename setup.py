#!/usr/bin/env python
# -*- mode: python; coding: utf-8 -*-

from commands import getoutput
from os.path import join as path_join

from distutils.core import setup, Extension

extra_compile_args = ['-std=gnu99', '-Wextra']
extra_compile_args.extend(getoutput('c-icap-libicapapi-config --cflags').split())

extra_link_args = getoutput('c-icap-libicapapi-config --libs').split()

ext = Extension(name='icapclient', sources=['icapclient.c', 'ICAPConnection.c', 'ICAPResponse.c'],
                extra_compile_args=extra_compile_args,
                extra_link_args=extra_link_args)

name_str = 'icapclient'
version_str = '1.0.0'
url_str = 'https://github.com/vrasneur/%s' % name_str
tarball_str = '%s/tarball/%s' % (url_str, version_str)

setup(name=name_str,
      version=version_str,
      description='Python module for creating ICAP clients',
      author='Vincent Rasneur',
      author_email='vrasneur@free.fr',
      url=url_str,
      download_url=tarball_str,
      keywords=['icap', 'antivirus'],
      ext_modules=[ext])
