# coasync4cpp Project  

coasync4cpp allows to write async code without callbacks using async/await/task! For C++ 11! 

[![Build Status](https://travis-ci.org/helgebetzinger/coasync4cpp.png?branch=dev-cmake)]
 (https://travis-ci.org/helgebetzinger/coasync4cpp)

## async/await 

This library let you use the async/await pattern, known from c#, for your c++ or qt projects. You can define any method as 'async' and than make use of the await or Task<> keywords of the library.  

Lets see in an simple example, what the library can do for you. In this QT-based example, we make use of QFuture and QFutureWatcher to await the result of an operation within the thread pool: 


     // Instantiate the objects and connect to the finished signal.
     MyClass myObject;
     QFutureWatcher<int> watcher;
     connect(&watcher, SIGNAL(finished()), &myObject, SLOT(handleFinished()));
     
     // Start the computation.
     QFuture<int> future = QtConcurrent::run(...);
     watcher.setFuture(future);

this can now be replaced by a single line of code: 
 
     int result = await QtConcurrent::run(...);

You save an complete callback, you have not longer to manage the QFuture/QFutureWatcher things. You simply write your code from top to bottom. As it is with synchronous code.

`QtConcurrent::run` returns here an `QFuture`. This is called an Awaitable. 

### Awaitables 

An operation, that is already running and promises to you to deliver an result in the future is called an 'Awaitable'. 

coasync4cpp currently supports a bunch of Awaitables: 
* boost::future
* QFuture
* Task<>

We can await an Awaitbale using the `Task<>` object or `await` keyword. But, these awaits are only allowed within an asyncronous execution context. To create such an context, we use Task Factories. 

### TaskFactories
### Thread Dispatcher
### How to start 

## Build 

### Requirements 

* [boost library 1.55 or newer](http://www.boost.org)
* qt library 5.0 or higher (only, if library will be used together with qt)
* [CMake 3.0.0 or higher](http://www.cmake.org/) 
* an c++ 11 compliant compiler

#### Windows specific Requirements 

An c++ 11 compliant compiler is here Microsoft Visual 12 2013 Express.

### Getting the Source

git clone 

### Build using CMake 

coasync4cpp comes with a CMake build script (CMakeLists.txt) that can
be used on a wide range of platforms ("C" stands for cross-platform.).
If you don't have CMake installed already, you can download it for
free from http://www.cmake.org/.

To help cmake resolvind dependencies, following environment variables must be defined, before cmake is used. Below this is an example 
how to define on windows. Replace the example paths with the correct paths on your system: 

     set BOOST_ROOT=C:\Projects\external_tools\boost\boost_1_55_0
     set BOOST_LIBRARYDIR=C:\Projects\external_tools\boost\boost_1_55_0\lib32-msvc-12.0
     set Qt5Core_DIR=C:\Qt\Qt5.3.0\5.3\msvc2013\lib\cmake\Qt5Core
     set Qt5Concurrent_DIR=C:\Qt\Qt5.3.0\5.3\msvc2013\lib\cmake\Qt5Concurrent

CMake works by generating native makefiles or build projects that can
be used in the compiler environment of your choice.  The typical
workflow starts with:

     mkdir mybuild       # Create a directory to hold the build output.
     cd mybuild
     cmake ${COASYNC_DIR}  # Generate native build scripts.

If you want to use coasync4cpp with qt, you have to build also coasync4qt. To do this you should replace the
last command with:

     cmake -Dbuild_coasync4qt=ON ${COASYNC_DIR}

If you want to build coasync4cpp/coasync4qt tests, you should replace the
last command with:

  cmake -Dbuild_tests=ON ${COASYNC_DIR}

A full call to cmake could than look like: 
    e.g.: C:\Projects\coasync4cpp\build>"C:\Program Files (x86)\CMake\bin\cmake" -Dbuild_coasync4qt=ON -Dbuild_tests=ON C:\Projects\coasync4cpp\build -G "Visual Studio 12 2013" 
  
If you are on a *nix system, you should now see a Makefile in the
current directory.  Just type 'make' to build gtest.

If you use Windows and have Vistual Studio installed, a coasync4cpp.sln file
and several .vcproj files will be created.  You can then build them
using Visual Studio.

On Mac OS X with Xcode installed, a .xcodeproj file will be generated.

## GitHub

[coasync4cpp issues](https://github.com/helgebetzinger/coasync4cpp/issues?q=is%3Aopen+sort%3Acreated-desc)

If you spot a bug or want to brainstorm a potential new feature, then please raise an issue in our main GitHub project (helgebetzinger/coasync4cpp); likewise if you have developed a cool new feature or improvement in your coasync4cpp fork, then send us a pull request!

For the wiki and readme.me, we use markdown syntax. We have good experiences using this [online editor](http://dillinger.io/).

## Documentation

* [Documentation within the coasync4cpp repository](https://github.com/helgebetzinger/coasync4cpp/tree/master/doc)
* [Library documentation of coasync4cpp](https://docs.google.com/document/d/1Ak2ZIMMJ6GRTIVOkbAHv2qeCym7z2GIcrmO93qsXPec/edit?usp=sharing)
* [Presentation on the Qt Developer Days Europe 2014](https://docs.google.com/presentation/d/1eWDEcOBHpcMdp16ZLsh8_Oj0edQK4WLviwiTQ6VMaYc)

## Community

[http://stackoverflow.com/](http://stackoverflow.com/)

If you found no answer here, then don't by shy and ask the community. Tag your question with an 'coasync4cpp' tag.

## Email

coasync4cpp@pcvisit.com

If you want to talk directly to us (e.g. about a commercially sensitive issue), email is the easiest way.

