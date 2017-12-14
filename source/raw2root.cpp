// raw2root.cpp
// C. R. Thornsberry
// June 15th, 2015
// Raw2Root.cpp
// SYNTAX: ./raw2root <filename> [options]

#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TNamed.h"

/** Find whether or not an integer is in a vector of ints.
  * Returns true if the integer is found and false otherwise.
  *  param[in] nums : Vector of integers to search.
  *  param[in] num_ : Integer to search for.
  */
bool is_in(const std::vector<int> &nums, int num_){
	for(unsigned int i = 0; i < nums.size(); i++){
		if(nums.at(i) == num_){ return true; }
	}
	return false;
}

/** Split an input string into a vector of strings based on a user specified delimiter.
  * Returns the number of substrings added to the vector.
  *  param[in] input_ : Input string to split into substrings.
  *  param[out] values : Vector of substrings which the input is split into.
  *  param[in] delimiter_ : The character around which to split the input string.
  */
size_t split_string(const std::string &input_, std::vector<std::string> &values, const char delimiter_='\t'){
	values.clear();
	
	size_t prevIndex = 0;
	size_t currIndex;
	while(true){
		currIndex = input_.find(delimiter_, prevIndex);
		
		if(currIndex == std::string::npos){
			values.push_back(input_.substr(prevIndex));
			break;
		}
			
		currIndex++;
		values.push_back(input_.substr(prevIndex, currIndex-prevIndex-1));
		
		if(currIndex == std::string::npos){ break; }
		prevIndex = currIndex;		
	}
	
	return values.size();
}

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> [options]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --help (-h)              | Display this dialogue.\n";
	std::cout << "    --names                  | First line of data file contains column names.\n";
	std::cout << "    --header <length>        | Number of lines in the data header.\n";
	std::cout << "    --delimiter <char>       | Supply the data delimiter for line parsing.\n";
	std::cout << "    --skip <num> <n1 n2 ...> | Skip num lines from the data file.\n";
	std::cout << "    --append                 | Append entries to an existing tree.\n";
}

int main(int argc, char* argv[]){
	if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		help(argv[0]);
		return 1;
	}
	else if(argc < 2){
		std::cout << " Error: Invalid number of arguments to " << argv[0] << ". Expected 1, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}

	std::vector<int> skip_lines;
	bool update_output_file = false;
	bool use_column_names = false;
	int index = 2;
	int num_head_lines = 0;
	int last_skip_line = -9999;
	char delimiter = '\t';
	while(index < argc){
		if(strcmp(argv[index], "--names") == 0){
			std::cout << " Using first line of data for column names\n";
			use_column_names = true;
		}
		else if(strcmp(argv[index], "--header") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--header'!\n";
				help(argv[0]);
				return 1;
			}
			num_head_lines = atoi(argv[++index]);
			std::cout << " Using file header of " << num_head_lines << " lines\n";
		}
		else if(strcmp(argv[index], "--delimiter") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--delimiter'!\n";
				help(argv[0]);
				return 1;
			}
			delimiter = (char)atoi(argv[++index]);
			std::cout << " Using column delimiter '" << delimiter << "'\n";
		}
		else if(strcmp(argv[index], "--skip") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--skip'!\n";
				help(argv[0]);
				return 1;
			}
			int num_lines_to_skip = atoi(argv[++index]);
			std::cout << " Skipping " << num_lines_to_skip << " lines in the data file\n";
			if(index + num_lines_to_skip >= argc){
				std::cout << " Error! Missing required argument to '--skip'!\n";
				help(argv[0]);
				return 1;
			}
			int temp_line;
			for(int i = 0; i < num_lines_to_skip; i++){
				temp_line = atoi(argv[++index]);
				skip_lines.push_back(temp_line);
				if(temp_line > last_skip_line){ last_skip_line = temp_line; }
			}
		}
		else if(strcmp(argv[index], "--append") == 0){
			update_output_file = true;
		}
		else{ 
			std::cout << " Error! Unrecognized option '" << argv[index] << "'!\n";
			help(argv[0]);
			return 1;
		}
		index++;
	}

	std::ifstream input_file(argv[1]);
	if(!input_file.good()){
		std::cout << " Error: Failed to open input file " << argv[1] << std::endl;
		return 1;
	}

	std::vector<std::string> filename;
	split_string(std::string(argv[1]), filename, '.');
	std::string fname = filename.front();
	TFile *output_file;
	if(!update_output_file)
		output_file = new TFile((fname+".root").c_str(),"RECREATE");
	else
		output_file = new TFile((fname+".root").c_str(),"UPDATE");
	if(!output_file->IsOpen()){
		std::cout << " Error: Failed to open output file " << fname << ".root\n";
		input_file.close();
		output_file->Close();
		return 1;
	}
	
	TTree *tree;
	if(!update_output_file)
		tree = new TTree("data","Raw2Root Tree");
	else{
		tree = (TTree*)output_file->Get("data");
		if(!tree){
			std::cout << " Error: Failed to load TTree \"data\" from file.\n";
			input_file.close();
			output_file->Close();
			return 1;
		}
	}
	unsigned int count = 0;
	unsigned int num_columns = 0;
	
	output_file->cd();

	std::vector<std::string> names;
	std::vector<std::string> titles;
	std::string first_line;
	if(num_head_lines > 0){
		for(int i = 0; i < num_head_lines; i++){
			std::getline(input_file, first_line);
			if(input_file.eof() || !input_file.good()){
				input_file.close();
				output_file->Close();
				return 1;
			}
			if(is_in(skip_lines, ++count)){ continue; }
			
			size_t index = first_line.find(delimiter);
			if(index != std::string::npos){
				names.push_back(first_line.substr(0, index));
				titles.push_back(first_line.substr(index+1));
			}
			else{ 
				std::stringstream stream;
				if(i < 9){ stream << "line00" << i+1; }
				else if(i < 99){ stream << "line0" << i+1; }
				else{ stream << "line" << i+1; }
				names.push_back(stream.str());
				titles.push_back(first_line);
			}
		}
	}

	// Get the number of columns
	std::getline(input_file, first_line);
	if(input_file.eof() || !input_file.good()){
		input_file.close();
		output_file->Close();
		return 1;
	}

	std::vector<std::string> column_names;
	num_columns = split_string(first_line, column_names, delimiter);
	std::cout << " Found " << num_columns << " columns of data\n";
	std::string bname;

	double *vars = new double[num_columns];
	for(unsigned int i = 0; i < num_columns; i++){
		vars[i] = 0.0;
	}

	if(!use_column_names){	
		input_file.seekg(-first_line.size(), std::ios::cur);
		for(unsigned int i = 0; i < num_columns; i++){
			std::stringstream stream;
			if(i < 10){ stream << "Col0" << i; }
			else{ stream << "Col" << i; }
			if(!update_output_file)
				tree->Branch(stream.str().c_str(),&vars[i]);
			else
				tree->SetBranchAddress(stream.str().c_str(),&vars[i]);
		}
	}
	else{ // Extract column names from data
		for(unsigned int i = 0; i < num_columns; i++){
			if(!update_output_file)
				tree->Branch(column_names[i].c_str(),&vars[i]);
			else{
				TBranch *branch;
				tree->SetBranchAddress(column_names[i].c_str(),&vars[i],&branch);
				if(!branch){
					std::cout << " Error: Failed to load branch \"" << column_names[i] << "\" from file.\n";
					input_file.close();
					output_file->Close();
					return 1;
				}
			}
		}
	}

	std::string line;
	std::vector<std::string> values;
	unsigned int total_counts = 0;
	while(true){		
		// Get a line of data
		getline(input_file, line);
		if(input_file.eof() || !input_file.good()){ break; }
	
		if(count % 10000 == 0 && count != 0){ std::cout << "  Line " << count << " of data file\n"; }
		if(count+1 <= (unsigned int)last_skip_line && is_in(skip_lines, count+1)){ 
			std::cout << " User skipping line " << ++count << std::endl;
			continue; 
		}
	
		// Split the data line into values.
		split_string(line, values, delimiter);
		count++;
		
		if(values.size() != num_columns){
			std::cout << " Skipping line " << count << std::endl;
			continue;
		}
		
		// Convert all line entries to doubles.
		for(unsigned int i = 0; i < num_columns; i++){
			vars[i] = strtod(values.at(i).c_str(), NULL);
		}
		
		// Fill all branches
		tree->Fill();
		total_counts++;
	}

	output_file->cd();
	TNamed *named = new TNamed();
	std::vector<std::string>::iterator iter1, iter2;
	for(iter1 = names.begin(), iter2 = titles.begin(); iter1 != names.end() && iter2 != titles.end(); iter1++, iter2++){
		named->SetNameTitle(iter1->c_str(), iter2->c_str());
		named->Write();
	}
	
	std::cout << " Found " << total_counts << " entries in " << num_columns << " columns\n";
	if(!update_output_file)
		std::cout << " Generated file " << fname << ".root\n";
	else
		std::cout << " Output tree now contains " << tree->GetEntries() << " entries\n";

	output_file->cd();
	tree->Write();

	input_file.close();
	output_file->Close();

	delete[] vars;

	return 0;
}
