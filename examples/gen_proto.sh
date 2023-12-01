SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
$SCRIPT_DIR/build/_deps/protobuf-build/Debug/protoc -I=. --cpp_out=.  example.proto