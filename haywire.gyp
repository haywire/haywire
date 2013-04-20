{
    'targets': [
      {
        'target_name': 'haywire',
        'type': 'executable',
        'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE65',
        'dependencies': [
          './lib/libuv/uv.gyp:libuv',
        ],
        'defines': [
          'DEFINE_FOO',
          'DEFINE_A_VALUE=value',
        ],
        'include_dirs': [
          './lib/libuv/include',
        ],
        'sources': [
          'src/program.c',
          'src/http_parser.c',
          'src/webserver.c'
        ],
        'conditions': [
          ['OS=="linux"', {
            'defines': [
              'LINUX_DEFINE',
            ],
            'include_dirs': [
              'include/linux',
            ],
          }],
          ['OS=="win"', {
            'defines': [
              'WINDOWS_SPECIFIC_DEFINE',
            ],
          }, { # OS != "win",
            'defines': [
              'NON_WINDOWS_DEFINE',
            ],
          }]
        ],
      },
    ],
  }