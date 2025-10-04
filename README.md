cd build
cmake ..
make
../bin/test

cd src/HTTP
ragel -C httprequest_parser.rl
ragel -C httprespond_parser.rl
ragel -C uri.rl