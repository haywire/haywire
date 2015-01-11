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

        'conditions': [
          ['OS=="linux"', {
            'defines': [
              'PLATFORM_LINUX',
              '_GNU_SOURCE',
            ],
            'cflags': [
              '-std=gnu99',
            ],
          }],
          ['OS=="win"', {
            'defines': [
              'PLATFORM_WINDOWS',
            ],
          }, { # OS != "win",
            'defines': [
              'PLATFORM_POSIX',
            ],
          }]
        ],

        'include_dirs': [
          './include',
          './lib/libuv/include',
        ],

        'sources': [
          'include/haywire.h',
          'src/haywire/configuration/configuration.h',
          'src/haywire/configuration/configuration.c',
          'src/haywire/configuration/ini.h',
          'src/haywire/configuration/ini.c',
          'src/haywire/connection_consumer.h',
          'src/haywire/connection_consumer.c',
          'src/haywire/connection_dispatcher.h',
          'src/haywire/connection_dispatcher.c',
          'src/haywire/http_connection.h',
          'src/haywire/http_parser.h',
          'src/haywire/http_parser.c',
          'src/haywire/http_request.h',
          'src/haywire/http_request.c',
          'src/haywire/http_response.h',
          'src/haywire/http_response.c',
          'src/haywire/http_response_cache.h',
          'src/haywire/http_response_cache.c',
          'src/haywire/http_server.h',
          'src/haywire/http_server.c',
          'src/haywire/hw_string.h',
          'src/haywire/hw_string.c',
          'src/haywire/khash.h',
          'src/haywire/route_compare_method.h',
          'src/haywire/route_compare_method.c',
          'src/haywire/server_stats.h',
          'src/haywire/server_stats.c',
        ],

      }, # haywire static library

      ########################################
      # haywire shared library
      ########################################
      {
        'target_name': 'haywire_shared',
        'product_name': 'haywire',
        'type': 'shared_library',
        'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE65',
        
        'dependencies': [
          './lib/libuv/uv.gyp:libuv',
        ],

        'conditions': [
          ['OS=="linux"', {
            'defines': [
              'PLATFORM_LINUX',
              '_GNU_SOURCE',
            ],
            'cflags': [
              '-std=gnu99',
            ],
          }],
          ['OS=="win"', {
            'defines': [
              'PLATFORM_WINDOWS',
              'BUILDING_HAYWIRE_SHARED',
            ],
          }, { # OS != "win",
            'defines': [
              'PLATFORM_POSIX',
            ],
          }]
        ],

        'include_dirs': [
          './include',
          './lib/libuv/include',
        ],

      'sources': [
          'include/haywire.h',
          'src/haywire/configuration/configuration.h',
          'src/haywire/configuration/configuration.c',
          'src/haywire/configuration/ini.h',
          'src/haywire/configuration/ini.c',
          'src/haywire/connection_consumer.h',
          'src/haywire/connection_consumer.c',
          'src/haywire/connection_dispatcher.h',
          'src/haywire/connection_dispatcher.c',          
          'src/haywire/http_connection.h',
          'src/haywire/http_parser.h',
          'src/haywire/http_parser.c',
          'src/haywire/http_request.h',
          'src/haywire/http_request.c',
          'src/haywire/http_response.h',
          'src/haywire/http_response.c',
          'src/haywire/http_response_cache.h',
          'src/haywire/http_response_cache.c',
          'src/haywire/http_server.h',
          'src/haywire/http_server.c',
          'src/haywire/hw_string.h',
          'src/haywire/hw_string.c',
          'src/haywire/khash.h',
          'src/haywire/route_compare_method.h',
          'src/haywire/route_compare_method.c',
          'src/haywire/server_stats.h',
          'src/haywire/server_stats.c',
        ],

      }, # haywire shared library

      ########################################
      # hello_world sample
      ########################################
      {
        'target_name': 'haywire_hello_world',
        'product_name': 'haywire_hello_world',
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

        'copies': [
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            'src/samples/hello_world/haywire_hello_world.conf',
          ],
        }],

      }, # hello_world sample

      ########################################
      # techempower_benchmark sample
      ########################################
      {
        'target_name': 'techempower_benchmark',
        'product_name': 'techempower_benchmark',
        'type': 'executable',
        'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE61',

        'dependencies': [
          'haywire',
        ],
        
        'include_dirs': [
          './include',
        ],

        'sources': [
          'src/samples/techempower_benchmark/program.c',
        ],

      }, # techempower_benchmark sample

    ],
  }
