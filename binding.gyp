{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'src/binding.cc',
      ],
      'dependencies': [
        'deps/libncurses/ncurses.gyp:libncurses',
      ],
      'cflags!': [ '-O2' ],
      'cflags+': [ '-O3' ],
      'cflags_cc!': [ '-O2' ],
      'cflags_cc+': [ '-O3' ],
      'cflags_c!': [ '-O2' ],
      'cflags_c+': [ '-O3' ],
    },
  ],
}
