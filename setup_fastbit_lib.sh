if [ -z ${SW_DIR} ]
then
  echo "set default SW_DIR: ${PWD}/sw/"
  SW_DIR=${PWD}/sw/
fi
# check if SW_DIR exists, if not make it
if [ ! -d ${SW_DIR} ]
then
  mkdir -p ${SW_DIR}
fi

if [ -d ${SW_DIR}/fastbit ]
then
  echo "fastbit is already installed in ${SW_DIR}"
else
  # fastbit
  fastbit_src_dir=$HOME/src/fastbit
  if [ -d ${fastbit_src_dir} ]
  then
    cd ${fastbit_src_dir}
    git fetch --prune
    git pull
  else
    git clone https://github.com/berkeleysdm/fastbit.git ${fastbit_src_dir}
    cd ${fastbit_src_dir}
  fi
  ./configure --prefix=${SW_DIR}/fastbit
  make -j32
  make install
fi

export FASTBIT_DIR=${SW_DIR}/fastbit
export FASTBIT_INCLUDE_DIR=${FASTBIT_DIR}/include
export FASTBIT_LIBRARY=${FASTBIT_DIR}/lib/libfastbit.so


# Echo variables for verification
echo "FASTBIT_DIR=${FASTBIT_DIR}"
echo "FASTBIT_INCLUDE_DIR=${FASTBIT_INCLUDE_DIR}"
echo "FASTBIT_LIBRARY=${FASTBIT_LIBRARY}"