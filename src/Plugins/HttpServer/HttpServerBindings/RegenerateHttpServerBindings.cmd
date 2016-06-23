setlocal EnableDelayedExpansion

cd..
doxygen HttpServerBindings\HttpServerBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe HttpServerBindings\HttpServerDocs\xml HttpServerBindings .  HttpServer HttpRequest