set -e
git clone https://github.com/jingtianer/dnsmasq_fuzz.git dnsmasq
SUBJECT=$PWD/dnsmasq

export FUZZER=$AFLFAST

export CC=$FUZZER/afl-clang-fast
export CXX=$FUZZER/afl-clang-fast++

export CFLAGS="-DENABLE_AFL=1"
export CXXFLAGS="-DENABLE_AFL=1"
export LDFLAGS="$CFLAGS -lpthread"

pushd $SUBJECT
    sudo make clean
	make -j
popd

tee dnsmasq.conf <<-'EOF'
port=5599
no-daemon
no-resolv
interface=lo
bind-interfaces
no-hosts
address=/test.com/5.5.5.5
EOF


export AFL_PERSISTENT=1
export FUZZ_SERVER_CONFIG=127.0.0.1:5599
$FUZZER/afl-fuzz -t 1000 -m none -i in -o out -- $SUBJECT/src/dnsmasq --conf-file=dnsmasq.conf