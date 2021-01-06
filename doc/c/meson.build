config = configuration_data()
config.set('PUGL_VERSION', meson.project_version())

conf_py = configure_file(configuration: config,
                         input: '../conf.py.in',
                         output: 'conf.py')

configure_file(copy: true, input: '../deployment.rst', output: 'deployment.rst')
configure_file(copy: true, input: '../summary.rst', output: 'summary.rst')

c_rst_files = files(
  'index.rst',
  'overview.rst',
  'world.rst',
  'view.rst',
  'events.rst',
  'event-loop.rst',
  'shutting-down.rst'
)

foreach f : c_rst_files
  configure_file(copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir('xml')
subdir('api')

docs = custom_target(
  'C API Documentation (singlehtml)',
  command: [sphinx_build, '-M', 'singlehtml', 'doc/c/', 'doc/c/', '-E', '-q', '-t', 'singlehtml'],
  input: [c_rst_files, c_pugl_rst, c_index_xml],
  output: 'singlehtml',
  build_by_default: true,
  install: true,
  install_dir: docdir / 'pugl-0')

docs = custom_target(
  'C API Documentation (html)',
  command: [sphinx_build, '-M', 'html', 'doc/c/', 'doc/c/', '-E', '-q', '-t', 'html'],
  input: [c_rst_files, c_pugl_rst, c_index_xml],
  output: 'html',
  build_by_default: true,
  install: true,
  install_dir: docdir / 'pugl-0')
