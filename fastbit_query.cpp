#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstdlib>

#include <adios2.h>
#include <fastbit/iapi.h>

/** A simple reader to be used by FastBit for index reconstruction.  In
    this simple case, the first argument is the whole array storing all the
    serialized bitmaps.  This first argument can be used to point to a data
    structure pointing to any complex object type necassary.
 */
static int mybmreader(void *ctx, uint64_t start, uint64_t count, uint32_t *buf) {
    unsigned j;
    const uint32_t *bms = (uint32_t*)ctx + start;
    for (j = 0; j < count; ++ j)
        buf[j] = bms[j];
    return 0;
} // mybmreader

int main(int argc, char *argv[])
{
    std::string dataFileName;
    std::string variableName;
    std::string indexFileName;
    int step;
    int blockID;
    std::string dataVarType;
    double low, high;

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
        else if (arg == "--step")
        {
            if (i+1 < argc)
            {
                step = atoi(argv[i+1]);
            }
            else
            {
                std::cerr << "--step option requires one argument." << std::endl;
                return 1;
            } 
        }
        else if (arg == "--block")
        {
            if (i+1 < argc)
            {
                blockID = atoi(argv[i+1]);
            }
            else
            {
                std::cerr << "--block option requires one argument." << std::endl;
                return 1;
            } 
        }
        else if (arg == "--value_range")
        {
            if (i+1 < argc)
            {
                low = atof(argv[i+1]);
                high = atof(argv[i+2]);
            }
            else
            {
                std::cerr << "--value_range option requires two arguments." << std::endl;
                return 1;
            } 
        }
    }
    fastbit_init(0);
    fastbit_set_verbose_level(0);

    adios2::ADIOS adios;
    adios2::IO index_reader_io = adios.DeclareIO("IndexReaderIO");
    adios2::Engine index_reader_engine = index_reader_io.Open(indexFileName, adios2::Mode::Read);

    adios2::IO data_reader_io = adios.DeclareIO("DataReaderIO");
    adios2::Engine data_reader_engine = data_reader_io.Open(dataFileName, adios2::Mode::Read);

    std::string bmsVarNameADIOS = "bms-"+variableName+"-"+std::to_string(step)+"-"+std::to_string(blockID);
    std::string keyVarNameADIOS = "key-"+variableName+"-"+std::to_string(step)+"-"+std::to_string(blockID);
    std::string offsetVarNameADIOS = "offset-"+variableName+"-"+std::to_string(step)+"-"+std::to_string(blockID);

    index_reader_engine.BeginStep();

    auto bmsVar = index_reader_io.InquireVariable(bmsVarNameADIOS);
    auto keyVar = index_reader_io.InquireVariable(keyVarNameADIOS);
    auto offsetVar = index_reader_io.InquireVariable(offsetVarNameADIOS);

    uint64_t nk=0, no=0, nb=0;
    nb = bmsVar.Shape()[0];
    nk = keyVar.Shape()[0];
    no = offsetVar.Shape()[0];

    std::vector<double> keys(nk);
    std::vector<int64_t> offsets(no);
    std::vector<uint32_t> bms(nb);

    index_reader_engine.Get(bmsVar, bms.data(), adios2::Mode::Sync);
    index_reader_engine.Get(keyVar, keys.data(), adios2::Mode::Sync);
    index_reader_engine.Get(offsetVar, offsets.data(), adios2::Mode::Sync);

    index_reader_engine.EndStep();

    data_reader_engine.BeginStep();
    long int ierr;
    auto dataVar = data_reader_io.InquireVariable(variableName);
    dataVarType = data_reader_io.VariableType(variableName);

    auto blocksInfo = data_reader_engine.AllStepsBlocksInfo(dataVar).at(step);
    size_t blockSize = 1;
    for (size_t i = 0; i < blocksInfo[blockID].Count.size(); i++)
    {
        blockSize *= blocksInfo[blockID].Count[i];
    } 
    uint64_t nv = blockSize;

    if (dataVarType == "double")
    {
        ierr = fastbit_iapi_register_array_index_only(variableName.c_str(), FastBitDataTypeDouble, &nv, 1, keys.data(), nk, offsets.data(), no, bms.data(), mybmreader);
        if (ierr < 0)
        {
            std::cerr << "Warning -- fastbit_iapi_attach_index failed to attach the index to a1, ierr = " << ierr << std::endl;
            return 1;
        }

        FastBitSelectionHandle h;
        h = fastbit_selection_combine(fastbit_selection_osr(variableName.c_str(), FastBitCompareGreater, low), FastBitCombineAnd, fastbit_selection_osr(variableName.c_str(), FastBitCompareLess, high));
        ierr = fastbit_selection_evaluate(h);
        int hits;
        if (ierr < 0)
        {
            std::cerr << "Warning -- fastbit_selection_evaluate(...) returned error code " << ierr << std::endl;
            return 1;
        }
        else 
        {
            hits = ierr;
            std::cout << "fastbit_selection_evaluate(...) returned " << hits << " hits" << std::endl;
        }

        uint64_t *coordinate;
        coordinate = (uint64_t *)malloc(sizeof(uint64_t)*nv);
        long int i;
        i = fastbit_selection_get_coordinates(h, coordinate, nv, 0);
        if (i < 0)
        {
            std::cerr << "Warning -- fastbit_selection_get_coordinates failed with error code " << i << std::endl;
            return 1;            
        }
        else
        {
            std::cout << "satisfied coordinates: ";
            for (size_t j = 0; j < hits; j++)
            {
                std::cout << coordinate[j] << " ";
            }
            std::cout << std::endl;
        }

        fastbit_selection_free(h);
        fastbit_iapi_free_all();

    }

    data_reader_engine.EndStep();
    
    index_reader_engine.Close();
    data_reader_engine.Close();

    return 0;

}