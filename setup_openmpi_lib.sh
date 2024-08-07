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

if [ -d ${SW_DIR}/openmpi ]
then
  echo "openmpi is already installed in ${SW_DIR}"
else
  # openmpi
  openmpi_src_dir=$HOME/src/openmpi
  if [ -d ${openmpi_src_dir} ]
  then
    cd ${openmpi_src_dir}
  else
    cd $HOME/src
    wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.3.tar.gz
    tar -xvf openmpi-5.0.3.tar.gz
    mv openmpi-5.0.3 openmpi
    cd openmpi
  fi
  ./configure --prefix=${SW_DIR}/openmpi
  make -j32
  make install
fi

export OPENMPI_DIR=${SW_DIR}/openmpi
export OPENMPI_INCLUDE_DIR=${OPENMPI_DIR}/include
export OPENMPI_LIBRARY=${OPENMPI_DIR}/lib/libopenmpi.so


# Echo variables for verification
echo "OPENMPI_DIR=${OPENMPI_DIR}"
echo "OPENMPI_INCLUDE_DIR=${OPENMPI_INCLUDE_DIR}"
echo "OPENMPI_LIBRARY=${OPENMPI_LIBRARY}"