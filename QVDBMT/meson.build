project('QVideoBrowser','cpp')
qtmmdep = dependency('Qt5MultimediaWidgets')
magickdep = dependency('Magick++-7.Q8')
add_global_arguments('-I.','-std=gnu++17','-march=native','-O3','-fPIC', language : 'cpp' )
executable('qvbrowser',['QVBrowser.cpp','DbConnector.cpp','VideoUtils.cpp','qmain.cpp'], dependencies : [qtmmdep,magickdep], link_args: ['-L/usr/lib/','-lavutil','-lswscale','-lavcodec','-lavformat','-lboost_timer','-lsqlite3','-lfftw3f','-lzstd','-pthread'])

executable('mdImporter',['DbConnector.cpp','mdImporter.cpp'], link_args: ['-L/usr/lib/','-lsqlite3','-pthread'])