#!/bin/sh

die() {
    echo "$@" >&2
    exit 1
}

BASE_DIR=$(cd "$(dirname "$0")"; pwd)
cd $BASE_DIR

if [ -z $1 ]
then

  # release
  echo -e "\033[33m begin compile \033[0m"
  rm build bin -rf && mkdir -p build bin

  (pushd build                                          \
      && cmake .. -DCMAKE_BUILD_TYPE="Release"          \
      && make -j2                                       \
      && popd) || die make "build"
  echo -e "\033[33m end compile \033[0m"

elif [ "x$1" == "xunittest" ]
then

  # unittest
  echo -e "\033[33m begin compile unittest \033[0m"
  rm build-unittest bin-unittest -rf && mkdir -p build-unittest bin-unittest

  (pushd build-unittest                                 \
      && cmake .. -DCMAKE_BUILD_TYPE="UnitTest"         \
      && make -j2                                       \
      && popd) || die make "build unittest"
  echo -e "\033[33m end compile unittest \033[0m"

elif [ "x$1" == "xcoverage" ]
then

  # coverage
  echo -e "\033[33m begin compile coverage \033[0m"
  rm build-unittest bin-unittest -rf && mkdir -p build-unittest bin-unittest

  (pushd build-unittest                                 \
      && cmake .. -DCMAKE_BUILD_TYPE="Coverage"         \
      && make -j2 && popd) || die make "build coverage" \
  echo -e "\033[33m end compile coverage \033[0m"

  (mkdir bin-unittest/coverage \
      && pushd bin-unittest    \
      && ./libevkv_test        \
      && lcov -c -o coverage/lcov.info -d .. --rc lcov_branch_coverage=1                                       \
      && lcov --remove coverage/lcov.info '${BASE_DIR}/src/kvclient/*' --output-file coverage/filter-lcov.info \
      && genhtml coverage/filter-lcov.info -o ${BASE_DIR}/bin-unittest/coverage                                \
      && popd) || die "run coverage"

else
    echo "invalid argument. you could run it as: ./autobuild.sh [unittest|coverage]"
fi
