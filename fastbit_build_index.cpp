#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstdlib>

#include <adios2.h>
#include <fastbit/iapi.h>

char* gBinningOption = 0;

void assertErr(long int errCode, const char* exp, const char* refName) 
{
  if (errCode < 0) {
    printf("errCode=%ld %s %s\n", errCode, exp, refName);
    exit(EXIT_FAILURE);
  } 
}

void fastbitIndex(const char* datasetName, void* data, uint64_t blockSize, FastBitDataType ft, 
		  double**keys, uint64_t*nk, int64_t**offsets, uint64_t*no,		  
		  uint32_t**bms, uint64_t*nb)
{
  long int fastbitErr;

  fastbitErr = fastbit_iapi_register_array(datasetName, ft, data, blockSize);
  assertErr(fastbitErr, "failed to register array with", datasetName);
  
  fastbitErr = fastbit_iapi_build_index(datasetName, (const char*)gBinningOption);
  assertErr(fastbitErr, "failed to build idx with ", datasetName);
  
  
  fastbitErr = fastbit_iapi_deconstruct_index(datasetName, keys, nk, offsets, no, bms, nb);
  assertErr(fastbitErr, "failed with fastbit_iapi_deconstruct on ", datasetName);
  
  //printf("nk/no/nb %" PRId64 " %" PRId64 " %" PRId64 "\n", *nk, *no, *nb);
  
  fastbit_iapi_free_all();

}


int main(int argc, char *argv[])
{
    std::string dataFileName;
    std::string variableName;
    std::string indexFileName;
    std::string variableType;

    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--data_file")
        {
            if (i+1 < argc)
            {
                dataFileName = argv[i+1];
            }
            else
            {
                std::cerr << "--data_file option requires one argument." << std::endl;
                return 1;
            }            
        }
        else if (arg == "--variable_name")
        {
            if (i+1 < argc)
            {
                variableName = argv[i+1];
            }
            else
            {
                std::cerr << "--variable_name option requires one argument." << std::endl;
                return 1;
            }             
        }
        else if (arg == "--index_file")
        {
            if (i+1 < argc)
            {
                indexFileName = argv[i+1];
            }
            else
            {
                std::cerr << "--index_file option requires one argument." << std::endl;
                return 1;
            }             
        }
    }
    fastbit_init(0);
    fastbit_set_verbose_level(0);

    adios2::ADIOS adios;
    adios2::IO reader_io = adios.DeclareIO("ReaderIO");
    adios2::Engine reader_engine = reader_io.Open(dataFileName, adios2::Mode::Read);
    size_t steps = reader_engine.Steps();


    
    adios2::IO writer_io = adios.DeclareIO("WriterIO");
    adios2::Engine writer_engine = writer_io.Open(indexFileName, adios2::Mode::Write);

    size_t step = 0;
    while (reader_engine.BeginStep() == adios2::StepStatus::OK)
    {
        auto var = reader_io.InquireVariable(variableName);
        variableType= reader_io.VariableType(variableName);
        if (variableType != "double")
        {
            std::cerr << "only double data type is supported" << std::endl;
            return 1;
        }
        
        auto blocksInfo = reader_engine.AllStepsBlocksInfo(var).at(step);
        size_t blockID = 0;
        std::cout << blockID << std::endl;
        for (const auto &info : blocksInfo)
        {
            char bmsVarName[100];
            char keyVarName[100];
            char offsetVarName[100];

            sprintf(bmsVarName, "bms-%s-%d-%d", variableName.c_str(), step, blockID);
            sprintf(keyVarName, "key-%s-%d-%d", variableName.c_str(), step, blockID);
            sprintf(offsetVarName, "offset-%s-%d-%d", variableName.c_str(), step, blockID);
            printf("bmsVarName: %s\n", bmsVarName);
            printf("keyVarName: %s\n", keyVarName);
            printf("offsetVarName: %s\n", offsetVarName);

            std::string bmsVarNameADIOS(bmsVarName);
            std::string keyVarNameADIOS(keyVarName);
            std::string offsetVarNameADIOS(offsetVarName);

            double* keys = NULL;
            int64_t *offsets = NULL;
            uint32_t *bms = NULL;
            uint64_t nk=0, no=0, nb=0;


            // std::cout << "    block " << blockID << " offset = ";
            // for (size_t i = 0; i < info.Start.size(); i++)
            // {
            //     std::cout << info.Start[i] << " ";
            // }
            size_t blockSize = 1;
            // std::cout << " size = ";
            for (size_t i = 0; i < info.Count.size(); i++)
            {
                // std::cout << info.Count[i] << " ";
                blockSize *= info.Count[i];
            }  
                        
            std::vector<double> blockData(blockSize);
            var.SetSelection({info.Start, info.Count});
            var.SetStepSelection({step, 1});
            reader_engine.Get(var, blockData.data(), adios2::Mode::Sync); 

            FastBitDataType ft = FastBitDataTypeDouble;
            fastbitIndex(variableName.c_str(), blockData.data(), blockSize, ft, &keys, &nk, &offsets, &no, &bms, &nb);

            auto bmsVar = writer_io.DefineVariable<uint32_t>(bmsVarNameADIOS, {nb}, {0}, {nb});
            auto keyVar = writer_io.DefineVariable<double>(keyVarNameADIOS, {nk}, {0}, {nk});
            auto offsetVar = writer_io.DefineVariable<int64_t>(offsetVarNameADIOS, {no}, {0}, {no});

            writer_engine.Put(bmsVar, bms);
            writer_engine.Put(keyVar, keys);
            writer_engine.Put(offsetVar, offsets);

            free(bms); bms = NULL;
            free(keys); keys = NULL;
            free(offsets); offsets = NULL;

            blockID++;
        }

        reader_engine.EndStep();
        step++;

    }

    reader_engine.Close();
    writer_engine.Close();

    return 0;
}