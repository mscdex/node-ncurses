{
  'defines': [
    'NDEBUG',
    'HAVE_CONFIG_H',
  ],
  'cflags!': [ '-O2' ],
  'cflags+': [ '-O3' ],
  'cflags_cc!': [ '-O2' ],
  'cflags_cc+': [ '-O3' ],
  'cflags_c!': [ '-O2' ],
  'cflags_c+': [ '-O3' ],
  'conditions': [
    [ 'OS=="linux"', {
      'include_dirs': [ 'linux' ],
    }],
    [ 'OS=="mac"', {
      'include_dirs': [ 'mac' ],
    }],
    [ 'OS=="freebsd"', {
      'include_dirs': [ 'freebsd' ],
    }],
  ],
}
