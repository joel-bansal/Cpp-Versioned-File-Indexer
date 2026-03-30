#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<unordered_map>
#include<algorithm>
#include<stdexcept>
#include<cctype>
#include<chrono>
#include <memory>

// Template function: Generic output printer for any data type
template <typename T>
void printOutput(const std::string& label, const T& value){
    std::cout<<label<<": "<<value<<'\n';
}

// Config Class: Parses and Stores the CLI arguments
class Config{
private:
    // File Paths and Version Identifiers 
    std::string filePath,file1,file2;
    std::string version_name,version1,version2;
    std::string query_type;
    std::string target_word;
    int k_value,bufferSizeInKB;
    int bufferSizeInBytes;

public:
    // Constructor initializes all varaibles from the command line
    Config(int argc,char* argv[])
    :bufferSizeInKB(512), k_value(0), bufferSizeInBytes(512*1024){

        for(int i=1;i<argc;i++){
            std::string arg = argv[i];

            if(arg == "--file" && i+1<argc){
                filePath = argv[++i];
            }
            else if(arg == "--file1" && i+1<argc){
                file1 = argv[++i];
            }
            else if(arg == "--file2" && i+1<argc){
                file2 = argv[++i];
            }
            else if(arg == "--version" && i+1<argc){
                version_name = argv[++i];
            }
            else if(arg == "--version1" && i+1<argc){
                version1 = argv[++i];
            }
            else if(arg == "--version2" && i+1<argc){
                version2 = argv[++i];
            }
            else if(arg == "--query" && i+1<argc){
                query_type = argv[++i];
            }
            else if(arg == "--word" && i+1<argc){
                target_word = argv[++i];
                std::transform(target_word.begin(), target_word.end(), target_word.begin(), ::tolower);
            }
            else if(arg == "--top" && i+1<argc){
                k_value = std::stoi(argv[++i]);
            }
            else if(arg == "--buffer" && i+1<argc){
                bufferSizeInKB = std::stoi(argv[++i]);
                if(bufferSizeInKB < 256 || bufferSizeInKB > 1024){
                    throw std::runtime_error("ERROR: Buffer Size must be between 256KB to 1024KB (both inclusive)");
                }
                bufferSizeInBytes = bufferSizeInKB*1024;
            }
        }

        // Throwing exceptions
        if(query_type.empty()){
            throw std::runtime_error("ERROR: --query is required (word | top | diff).");
        }
        if(query_type == "diff"){
            if(file1.empty() || file2.empty() || version1.empty() || version2.empty()){
                throw std::runtime_error("ERROR: diff query needs --file1 --version1 --file2 --version2.");
            }
            if(target_word.empty()){
                throw std::runtime_error("ERROR: --word is required for diff query.");
            }
        }
        else {
            if(filePath.empty()){
                throw std::runtime_error("ERROR: --file is required.");
            }
            if(version_name.empty()){
                throw std::runtime_error("ERROR: --version is required.");
            }
            if(query_type == "word" && target_word.empty()){
                throw std::runtime_error("ERROR: --word is required for word query.");
            }
            if(query_type == "top" && k_value <= 0){
                throw std::runtime_error("ERROR: --top <k> is required and must be > 0 for top query.");
            }
        }
    }

    // Member Functions to get all details from CLI as and when needed 
    std::string getFilePath() const;
    std::string getFile1() const;
    std::string getFile2() const;
    std::string getVersionName() const;
    std::string getVersion1() const;
    std::string getVersion2() const;
    std::string getQuery() const;
    std::string getWord() const;
    int getK() const;
    int getBufferSizeKB() const;
    int getBufferSizeBytes() const;
};

// VersionedIndex Class: Maintains a versioned word frequency Index
class VersionedIndex{
private:
    // Maps version name to corresponding unordered map 
    std::map<std::string, std::unordered_map<std::string,int>> frequencyMap;

public: 
    // Functions to update frequencies for the versions and return data about those versions
    void updateFreq(const std::string&word, const std::string& version);
    int getFrequency(const std::string& word,const std::string& v1) const;
    const std::unordered_map<std::string, int>& getVersionData(const std::string& v) const;
    bool versionExists(const std::string& version) const;
};

// Tokenizer Class extracts words(in lower case) from texts updates in the maps
class Tokenizer{
private:
    VersionedIndex& vi;

public: 
    Tokenizer(VersionedIndex& vi_ref) : vi(vi_ref){}
    // Overloaded process: Tokenizes text and updates index for a version
    void process(const std::string& text,const std::string& version);
    // Overloaded process: Tokenizes text with chunk offset information (for debugging)
    void process(const std::string& text, const std::string& version, size_t chunkOffset);
};

// BufferedReader Class Reads large files incrementally using a fixed size buffer 
class BufferedReader{
private:
    Tokenizer& token;
    size_t bufferSizeInBytes; // Fixed Buffer Size

public:
    BufferedReader(Tokenizer& t_ref, size_t bufferInBytes)
        : token(t_ref), bufferSizeInBytes(bufferInBytes){}

    // Reads file in fixed-size buffer chunks and tokenizes each chunk
    void readFile(const std::string& path, const std::string& version);
};

// Query Class is the parent class for the 3 types of queries. Defines Interface for all types.
class Query{
public:
    virtual ~Query(){}
    // Pure Virtual Function
    virtual void execute() = 0;
};

// WordDiffQuery Class for computing differneces in word frequencies in 2 separate versions
class WordDiffQuery: public Query{
private:
    std::string word;
    std::string v1,v2;
    VersionedIndex& indexer;

public:
    WordDiffQuery(std::string w,std::string ver1,std::string ver2,VersionedIndex& idx):
        word(w),v1(ver1),v2(ver2), indexer(idx){};
    
    // Executes the difference Operation 
    void execute() override;
};

// WordQuery Class Retrieves word frequency from a single version
class WordQuery: public Query{
private:
    std::string word;
    std::string v1;
    VersionedIndex& indexer;

public:
    WordQuery(std::string w,std::string v,VersionedIndex& idx):word(w),v1(v), indexer(idx){};

    // Executes word query: Retrieves and prints frequency of a word in a version
    void execute() override;
};

// TopKQuery Class Retrieves and displays top-K most frequent words 
class TopKQuery: public Query{
private:
    std::string v1;
    int k;
    VersionedIndex& indexer;

public:
    TopKQuery(std::string v,int k_in,VersionedIndex& idx):v1(v), k(k_in), indexer(idx){};

    // Executes top-K query: Retrieves top-K words sorted by frequency (descending)
    void execute() override;
};

// QueryProcessor Class Drives the entire file indexing and query execution pipeline, and measures time of execution
// Coordinates BufferedReader, Tokenizer, VersionedIndex, and Query execution
class QueryProcessor{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    int buffer_size;

    Config& cfg;
    VersionedIndex vi;
    Tokenizer t;
    BufferedReader br;

public:
    QueryProcessor(Config& cf) : cfg(cf), buffer_size(cf.getBufferSizeKB()), vi(), t(vi), br(t,cf.getBufferSizeBytes()){
        start_time = std::chrono::high_resolution_clock::now();
    }

    // Logs informational messages to stderr
    void log(const std::string& msg){
        std::cerr<<"[INFO]: "<<msg<<'\n';
    }

    // Logs informational messages with numeric values
    void log(const std::string& msg, double val){
        std::cerr<<"[INFO]: "<<msg<<" "<<val<<'\n';
    }

    // Template function: Generic output printer for any data type
    template <typename T>
    void printOutput(const std::string& label, const T& value){
        std::cout<<label<<": "<<value<<'\n';
    }

    // Reads files, builds index, and executes query
    void run();

    // Calculates and prints execution time and buffer size
    ~QueryProcessor(){
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        printOutput("Buffer Size (KB)", buffer_size);
        printOutput("Execution Time (s)", elapsed.count());
    }
};

// ----------------------------MEMBER FUNCTIONS DEFINITIONS----------------------------

// Config Member Functions
std::string Config::getFilePath() const{
    return filePath;
}
std::string Config::getFile1() const{
    return file1;
}
std::string Config::getFile2() const{
    return file2;
}
std::string Config::getVersionName() const{
    return version_name;
}
std::string Config::getVersion1() const{
    return version1;
}
std::string Config::getVersion2() const{
    return version2;
}
std::string Config::getQuery() const{
    return query_type;
}
std::string Config::getWord() const{
    return target_word;
}
int Config::getK() const{
    return k_value;
}
int Config::getBufferSizeKB() const{
    return bufferSizeInKB;
}
int Config::getBufferSizeBytes() const{
    return bufferSizeInBytes;
}

// VersionedIndex Member Functions
void VersionedIndex::updateFreq(const std::string&word, const std::string& version){
    frequencyMap[version][word]++;
}

int VersionedIndex::getFrequency(const std::string& word,const std::string& v1) const{
    if (frequencyMap.find(v1) == frequencyMap.end()){
        throw std::runtime_error("Version not found.");
    }
    const auto& innerMap = frequencyMap.at(v1);
    auto it = innerMap.find(word);
    
    if(it!=innerMap.end()){
        return it->second;
    }
    return 0;
}

const std::unordered_map<std::string, int>& VersionedIndex::getVersionData(const std::string& v) const {
    if (frequencyMap.find(v) == frequencyMap.end()){
        throw std::runtime_error("Version not found.");
    }
    return frequencyMap.at(v);
}

bool VersionedIndex::versionExists(const std::string& version) const{
    return frequencyMap.find(version) != frequencyMap.end();
}

// Tokenizer Member Functions
void Tokenizer::process(const std::string& text,const std::string& version){
    std::string word = "";

    for(int i=0;i<text.size();i++){
        if(std::isalnum(static_cast<unsigned char>(text[i]))){
            word += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
        }
        else{
            if(!word.empty()){
                vi.updateFreq(word,version);
                word = "";
            }
        }
    }
    if(!word.empty()){
        vi.updateFreq(word,version);
    }
}

void Tokenizer::process(const std::string& text, const std::string& version, size_t chunkOffset){
    std::cerr<<"DEBUG: Processing chunk at offset "<<chunkOffset << "\n";
    process(text, version);
}

// BufferedReader Member Functions
void BufferedReader::readFile(const std::string& path, const std::string& version){
    try{
        std::ifstream file(path);
        if(!file.is_open()){
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::string prev_left_string = "";
        std::unique_ptr<char[]> buffer(new char[bufferSizeInBytes]);

        while(file.read(buffer.get(), bufferSizeInBytes) || file.gcount() > 0){
            auto bytesRead = file.gcount();
            if(bytesRead == 0) break;

            std::string curr_chunk(buffer.get(), bytesRead);
            std::string combined = prev_left_string + curr_chunk;

            size_t last_boundary = combined.size();
            for(int i = combined.size()-1; i >= 0; i--){
                if(!std::isalnum(static_cast<unsigned char>(combined[i]))){
                    last_boundary = i;
                    break;
                }
            }

            std::string safe_to_process = combined.substr(0, last_boundary);
            prev_left_string = combined.substr(last_boundary);

            token.process(safe_to_process, version);
        }

        if(!prev_left_string.empty()){
            token.process(prev_left_string, version);
        }
    }
    catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

// WordDiffQuery Member Functions
void WordDiffQuery::execute(){
    const auto& data1 = indexer.getVersionData(v1);
    const auto& data2 = indexer.getVersionData(v2);

    int count1 = data1.count(word) ? data1.at(word) : 0;
    int count2 = data2.count(word) ? data2.at(word) : 0;

    printOutput("Version 1", v1);
    printOutput("Count", count1);

    printOutput("Version 2", v2);
    printOutput("Count", count2);
    printOutput("Difference (" + v2 + " - " + v1 + ")", count2-count1);
}

// WordQuery Member Functions
void WordQuery::execute(){
    int count = indexer.getFrequency(word,v1);
    printOutput("Version", v1);
    printOutput("Count", count);
}

// Teamplate Function for TopKQuery Class
template<typename MapType>
std::vector<std::pair<std::string, typename MapType::mapped_type>>
getTopK(const MapType& m, int k){
    std::vector<std::pair<std::string, typename MapType::mapped_type>> vec(m.begin(), m.end());
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b){
        return a.second > b.second;
    });
    if(k < (int)vec.size()) vec.resize(k);
    return vec;
}

// TopKQuery Member Functions
void TopKQuery::execute(){
    const auto& data = indexer.getVersionData(v1);

    auto results = getTopK(data, k);
    
    printOutput("Version", v1);
    printOutput("Top " + std::to_string(k) + " words in Version", v1);
    for (int i = 0; i < (int)results.size(); ++i) {
        printOutput(results[i].first, results[i].second);
    }
}

// QueryProcessor Member Functions
void QueryProcessor::run(){
    std::string query = cfg.getQuery();

    // log("Buffer Size (KB):", (double)buffer_size);

    if(query == "diff"){
        br.readFile(cfg.getFile1(), cfg.getVersion1());
        br.readFile(cfg.getFile2(), cfg.getVersion2());
    }
    else{
        br.readFile(cfg.getFilePath(), cfg.getVersionName());
    }

    // Using Smart Pointers so as to avoid dangling pointers in the case of an exception
    std::unique_ptr<Query> q;

    if(query == "diff"){
        if(!vi.versionExists(cfg.getVersion1()))
            throw std::runtime_error("Version not indexed: " + cfg.getVersion1());
        if(!vi.versionExists(cfg.getVersion2()))
            throw std::runtime_error("Version not indexed: " + cfg.getVersion2());
    }
    else{
        if(!vi.versionExists(cfg.getVersionName()))
            throw std::runtime_error("Version not indexed: " + cfg.getVersionName());
    }

    if(query == "word"){
        std::string w = cfg.getWord();
        std::string v = cfg.getVersionName();
        q = std::make_unique<WordQuery>(w, v, vi);
    }
    else if(query == "top"){
        q = std::make_unique<TopKQuery>(cfg.getVersionName(), cfg.getK(), vi);
    }
    else if(query == "diff"){
        q = std::make_unique<WordDiffQuery>(cfg.getWord(), cfg.getVersion1(), cfg.getVersion2(), vi);
    }
    else{
        throw std::runtime_error("Unknown query type: " + query);
    }

    q->execute();
}

// Main Function with proper try catch throw
int main(int argc,char* argv[]){
    try{
        Config cfg(argc,argv);
        QueryProcessor qp(cfg);
        qp.run();

    }
    catch(const std::invalid_argument& e){
        std::cerr<<"INPUT ERROR: Expected a number but got a string (stoi failed)."<<std::endl;
        return 1;
    }
    catch(const std::out_of_range& e){
        std::cerr<<"INPUT ERROR: The number you passed is too large."<<std::endl;
        return 1;
    }
    catch(const std::exception& e){
        std::cerr<<"CAUGHT EXCEPTION: "<<e.what()<<std::endl;
        return 1;
    }
    catch(...){
        std::cerr<<"UNKNOWN CRASH: Something went wrong."<<std::endl;
        return 1;
    }
    return 0;
}
