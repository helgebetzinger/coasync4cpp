# coasync4cpp Project  

coasync4cpp allows to write async code without callbacks using async/await/task! For C++11! 

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

`QtConcurrent::run` returns here an `QFuture`. This is called an Awaitable. coasync4cpp currently supports a bunch of Awaitables: 
* boost::future
* QFuture
* Task<>


### Awaitables 
### TaskFactories
### Thread Dispatcher
### How to start 


## GitHub

[coasync4cpp issues](https://github.com/helgebetzinger/coasync4cpp/issues?q=is%3Aopen+sort%3Acreated-desc)

If you spot a bug or want to brainstorm a potential new feature, then please raise an issue in our main GitHub project (helgebetzinger/coasync4cpp); likewise if you have developed a cool new feature or improvement in your coasync4cpp fork, then send us a pull request!

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

