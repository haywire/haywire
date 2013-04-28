{
    'targets': [
      
      ########################################
      # haywire static library
      ########################################
      {
        'target_name': 'haywire',
        'product_name': 'haywire',
        'type': 'static_library',
        'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE65',
        
        'dependencies': [
          './lib/libuv/uv.gyp:libuv',
        ],

        'include_dirs': [
          './lib/libuv/include',
        ],

        'sources': [
          'include/haywire.h',
          'src/haywire/http_parser.h',
          'src/haywire/http_parser.c',
          'src/haywire/http_request.h',
          'src/haywire/http_request.c',
          'src/haywire/http_server.h',
          'src/haywire/http_server.c'
        ],
      }, # haywire static library

      ########################################
      # hello_world sample
      ########################################
      {
        'target_name': 'hello_world',
        'product_name': 'hello_world',
        'type': 'executable',
        'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE66',

        'dependencies': [
          'haywire',
        ],
        
        'include_dirs': [
          './include',
        ],

        'sources': [
          'src/samples/hello_world/program.c',
        ],
      }, # hello_world sample

    ],
  }