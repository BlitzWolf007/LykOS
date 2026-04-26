project(
    'apps',
    'c',
    version: '0.1',
    meson_version: '>=0.64.0',
    default_options: [
        'c_std=c23',
        'warning_level=3',
        'buildtype=debug',
    ],
)

app_install_dir = 'bin'
test_install_dir = 'tests'

common_c_args = [
    '-O0',
    '-g',
]

common_link_args = [
    '-static',
    '-no-pie',
]

subdir('console')
subdir('init')
# subdir('tests')
