project('QVideoBrowser','cpp', default_options : 'cpp_std=c++17')
qtmmdep = dependency('Qt5MultimediaWidgets')
magickdep = dependency('Magick++-7.Q8')
threaddep = dependency('threads')
boostdep = dependency('boost', modules : ['timer'])
dlibdep = dependency('dlib-1')
add_global_arguments('-fPIC', language : 'cpp' )
executable('qvbrowser',['QVBrowser.cpp','DbConnector.cpp','QVDec.cpp','VideoUtils.cpp','FaceTools.cpp','qmain.cpp'], dependencies : [qtmmdep,magickdep,threaddep,boostdep,dlibdep], link_args: ['-L/usr/lib/','-lavutil','-lswscale','-lavcodec','-lavformat','-lsqlite3','-lfftw3f','-lzstd','-lpng','-ldlib'])