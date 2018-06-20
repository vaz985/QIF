#include <algorithm>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <random>
#include <sstream>

#include "channel.h"

namespace channel {


Channel::Channel(int n_in, int n_out) : n_in_(n_in), n_out_(n_out) {
  this->Reset();

  // Should we call Identity instead?
  this->Randomize();
}

Channel::Channel(const std::vector<std::vector<double> > & c_matrix,
                 int base_norm) {
  this->n_in_  = c_matrix.size();
  this->n_out_ = c_matrix[0].size();
  this->base_norm_ = base_norm;
  this->build_channel(c_matrix);
}

Channel::Channel(const std::vector<std::vector<double> > & c_matrix,
                std::vector<double> prior_distribution,
                int base_norm) {
  this->n_in_  = c_matrix.size();
  this->n_out_ = c_matrix[0].size();
  this->base_norm_ = base_norm;
  this->build_channel(c_matrix, prior_distribution);
}


// This function resets the class to an initial state.
void Channel::Reset() {
  // The prior is a uniform distribution by default.
  this->prior_distribution_.assign(this->n_in_, 1.0f/this->n_in_);
  this->out_distribution_.assign(this->n_out_, 0);

  this->max_pinput_.assign(this->n_in_, 0);

  this->max_poutput_.assign(this->n_out_, 0);

  // Important: The first index of the matrices always represents
  // the x variable.
  this->c_matrix_.assign(this->n_in_, std::vector<double>(this->n_out_, 0));
  this->h_matrix_.assign(this->n_in_, std::vector<double>(this->n_out_, 0));
  this->j_matrix_.assign(this->n_in_, std::vector<double>(this->n_out_, 0));
}


void Channel::ParseFile(std::string fname) {
  std::ifstream f(fname);
  std::string contentFile;
  std::string curr_line;
  while(std::getline(f, curr_line)) {
    contentFile += curr_line; 
    contentFile += '\n';
  }
  ParseInput(contentFile); 
  f.close();
}


// This function returns a string that represents the
// current channel.
std::string Channel::to_string() const {
  std::stringstream ss;

  ss << std::fixed << std::setprecision(8);
  
  ss << this->cname_ << std::endl;
  ss << this->n_in_ << " " << this->n_out_ << std::endl;
  ss << this->base_norm_ << std::endl;
  
  for(std::string s : this->in_names())
    ss << s << " ";
  ss << std::endl;

  for(std::string s : this->out_names())
    ss << s << " ";
  ss << std::endl;

  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++)
      ss << this->c_matrix_[i][j] << " ";
    ss << std::endl;
  }

  for(int i = 0; i < this->n_in_; i++) {
    ss << this->prior_distribution_[i] << " ";
  }
  ss << std::endl;

  return ss.str();
}


bool Channel::CompatibleChannels(const Channel& c1, const Channel& c2) {
  return (c1.n_in() == c2.n_in() && c1.in_names() == c2.in_names());
}

std::ostream& operator<< (std::ostream& stream, const Channel& channel) {
  stream << channel.to_string();
  return stream;
}

double Channel::ShannonEntropyPrior() const {
  double entropy = 0;
  for(int i = 0; i < this->n_in_; i++) {
    if(this->prior_distribution_[i] != 0)
      entropy += (this->prior_distribution_[i]*log2(1.0f/this->prior_distribution_[i]));
  }
  return entropy;
}


void Channel::build_channel(std::vector<std::vector<double> > c_matrix) {
  this->n_in_  = c_matrix.size();
  this->n_out_ = c_matrix[0].size();
  this->prior_distribution_ = std::vector<double>(this->n_in_,
                                                  1.0f/this->n_in_);
  this->build_channel(c_matrix, this->prior_distribution_);
}



void Channel::build_channel(std::vector<std::vector<double> > c_matrix,
    std::vector<double> prior_distribution) {
  this->n_in_  = c_matrix.size();
  this->n_out_ = c_matrix[0].size();
  this->Reset();
  this->c_matrix_ = c_matrix;
  this->prior_distribution_ = prior_distribution;

  
  // Filling j_matrix
  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      this->j_matrix_[i][j] = this->c_matrix_[i][j] * this->prior_distribution_[i];
    }
  }

  // Filling outdistribution, maxpinput and maxpoutput
  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      this->out_distribution_[j] += this->j_matrix_[i][j];
      this->max_pinput_[i] = std::max(this->max_pinput_[i],
                                     this->j_matrix_[i][j]);
      this->max_poutput_[j] = std::max(this->max_poutput_[j],
                                       this->j_matrix_[i][j]);
    }
  }
  
  // Filling h_matrix
  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      this->h_matrix_[i][j] = this->j_matrix_[i][j]/this->out_distribution_[j];
    }
  }
}


// Parallel Operator
Channel operator||(const Channel & c1, const Channel & c2) {
  if(!Channel::CompatibleChannels(c1,c2)) {
    std::cout << "Channels not compatible" << std::endl;
    std::cout << c1 << std::endl;
    std::cout << c2 << std::endl;
    return c1;
  }
  std::vector<std::string> out_;
  std::vector<std::vector<double> > c_m(c1.n_in());
  for(int i=0; i<c1.n_in(); i++) {
    c_m[i].assign(c1.n_out() * c2.n_out(), 0);
  }

  std::vector<std::vector<double> > c1_c = c1.c_matrix();
  std::vector<std::vector<double> > c2_c = c2.c_matrix();

  int col_pos = 0;
  for(int i=0; i<c1.n_out(); i++) {
    for(int j=0; j<c2.n_out(); j++) {
      std::string c1_;
      std::string c2_;
      c1_ = c1.out_names()[i];
      c2_ = c2.out_names()[j];

      out_.push_back(c1_ + '.' + c2_);
      for(int k=0; k<c1.n_in(); k++) {
        c_m[k][col_pos] = c1_c[k][i] * c2_c[k][j];
      }
      col_pos++;
    }
  }
  Channel c3(c_m);
  c3.set_in_names(c1.in_names());
  c3.set_out_names(out_);
  return c3;
}


// Cascade
Channel operator*(const Channel& c1, const Channel& c2) {
  std::vector<std::vector<double> > new_c(c1.n_in());
  std::vector<std::vector<double> > c1_c = c1.c_matrix();
  std::vector<std::vector<double> > c2_c = c2.c_matrix();
  for(int i=0; i<c1.n_in(); i++) {
    new_c[i].resize(c2.n_out());
    for(int j=0; j<c2.n_out(); j++) {
      double sum = 0;
      for(int k=0; k<c1.n_out(); k++) sum += (c1_c[i][k] * c2_c[k][j]);
      new_c[i][j] = sum;
    }
  }
  // Is the prior of c3 always uniform?
  Channel c3(new_c);
  c3.set_in_names(c1.in_names());
  c3.set_out_names(c2.out_names());
  for(int i=0; i<(int)c2.out_names().size(); i++)
    c3.insert_out_index(c3.out_names()[i], i); 
  return c3;
}

Channel Channel::hidden_choice (const Channel& c1, const Channel& c2, const double prob) {
  std::vector<std::vector<double> > c_m(c1.n_in());
  std::vector<std::vector<double> > c1_m = c1.c_matrix();
  std::vector<std::vector<double> > c2_m = c2.c_matrix();

  std::vector<std::string> c1_n = c1.out_names();
  std::vector<std::string> c2_n = c2.out_names();

  std::vector<std::string> union_out_names(c1_n);
  union_out_names.insert(union_out_names.end(), c2_n.begin(), c2_n.end());

  // Removing duplicates
  sort(union_out_names.begin(), union_out_names.end());
  union_out_names.erase( unique(union_out_names.begin(), union_out_names.end()), 
                         union_out_names.end());


  for(int i=0; i<c1.n_in(); i++) {
    c_m[i].assign(union_out_names.size(), 0);
    for(int j=0; j<(int)union_out_names.size(); j++) {
      bool f1 = false;
      bool f2 = false;

      int c1_j, c2_j;

      if( find(c1_n.begin(), c1_n.end(), union_out_names[j]) != c1_n.end() ) 
        f1 = true;
      if( find(c2_n.begin(), c2_n.end(), union_out_names[j]) != c2_n.end() )
        f2 = true;

      if( f1 && f2 ) {
        c1_j = c1.out_index(union_out_names[j]);
        c2_j = c2.out_index(union_out_names[j]);
        c_m[i][j] = (prob)*(c1_m[i][c1_j]) + (1-prob)*(c2_m[i][c2_j]);
      }
      else if( f1 ) {
        c1_j = c1.out_index(union_out_names[j]);
        c_m[i][j] = (prob)*(c1_m[i][c1_j]);
      }
      else if( f2 ) {
        c2_j = c2.out_index(union_out_names[j]);
        c_m[i][j] = (1-prob)*(c2_m[i][c2_j]);
      }
      else 
        std::cout << "deu ruim no find" << std::endl;
    }
  }
  Channel c3(c_m);
  c3.set_in_names(c1_n);
  c3.set_out_names(union_out_names);
  for(int i=0; i<(int)union_out_names.size(); i++)
    c3.insert_out_index(union_out_names[i], i);
  return c3; 
}

// This function parses a channel string.
void Channel::ParseInput(std::string input_str) {
  std::stringstream f;
  f << input_str;

  f >> this->cname_; 
  f >> this->n_in_;
  f >> this->n_out_;

  // Initializing every class property.
  this->Reset();

  std::string s;
  this->in_names_.resize(this->n_in_);
  this->out_names_.resize(this->n_out_);

  for(int i=0; i<this->n_in_; i++)  pos_in_names_[in_names_[i]] = i;
  for(int i=0; i<this->n_out_; i++) pos_out_names_[out_names_[i]] = i;

  for(int i = 0; i < this->n_in_; i++) {
    f >> this->in_names_[i];
    this->pos_in_names_[this->in_names_[i]] = i;
  }
  for(int i = 0; i < this->n_out_; i++) { 
    f >> this->out_names_[i];
    this->pos_out_names_[this->out_names_[i]] = i;
  }
  for(int i = 0; i < this->n_in_; i++)
    for(int j = 0; j < this->n_out_; j++)
      f >> this->c_matrix_[i][j];

  for(int i = 0; i < this->n_in_; i++)
    f >> this->prior_distribution_[i]; 

  this->build_channel(this->c_matrix_, this->prior_distribution_);
}


// This function randomizes the current channel.
// Maintaining the channel dimensions.
void Channel::Randomize() {
  // RNG
  std::mt19937 rng;
  rng.seed(std::random_device()());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0,1000);

  this->base_norm_ = 0;
  this->prior_distribution_.assign(this->n_in_, 0);
  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      int neue = dist(rng);
      this->base_norm_ += neue;
      this->c_matrix_[i][j] = neue;
      this->prior_distribution_[i] += neue;
    }
  }

  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      if(this->prior_distribution_[i] != 0) {
        this->c_matrix_[i][j] /= this->prior_distribution_[i];
      }
      else {
        this->c_matrix_[i][j] = 0;
      }
      this->prior_distribution_[i] /= this->base_norm_;
    }
  }

  this->build_channel(this->c_matrix_, this->prior_distribution_);
}


// Start metrics
double Channel::ShannonEntropyOut() const {
  double entropy = 0;
  for(int i = 0; i < this->n_out_; i++) {
    if(this->out_distribution_[i] != 0)
      entropy += (this->out_distribution_[i]*log2(1.0f/this->out_distribution_[i]));
  }
  return entropy;
}

// H(X|Y)
double Channel::ConditionalEntropyHyper() const {
  double entropy = 0;
  for(int j = 0; j < this->n_out_; j++) {
    double conditional_entropy_X = 0;
    for(int i = 0; i < this->n_in_; i++) {
      if(this->h_matrix_[i][j] != 0)
        conditional_entropy_X += (this->h_matrix_[i][j] * log2(1.0f/this->h_matrix_[i][j]));
    }
    entropy += (this->out_distribution_[j] * conditional_entropy_X);
  }
  return entropy;
}


// H(Y|X)
double Channel::ConditionalEntropy() const {
  double entropy = 0;
  for(int i = 0; i < this->n_in_; i++) {
    double conditional_entropy_Y = 0;
    for(int j = 0; j < this->n_out_; j++) {
      if(this->c_matrix_[i][j] != 0)
        conditional_entropy_Y += (this->c_matrix_[i][j] * log2(1.0f/this->c_matrix_[i][j]));
    }
    entropy += (this->prior_distribution_[i] * conditional_entropy_Y);
  }
  return entropy;

}

double Channel::JointEntropy() const {
  double entropy = 0;
  for(int i = 0; i < this->n_in_; i++) {
    for(int j = 0; j < this->n_out_; j++) {
      if(this->j_matrix_[i][j] != 0)
        entropy += (this->j_matrix_[i][j]*log2(1.0f/this->j_matrix_[i][j]));
    }
  }
  return entropy;
}

double Channel::GuessingEntropy() const {
  double entropy = 0;
  std::vector<double> v = this->prior_distribution_;
  std::sort(v.begin(),v.end());
  std::reverse(v.begin(), v.end());
  for(int i = 1; i <= this->n_in_; i++)
    entropy += (i*v[i-1]);
  return entropy;
}

// 
double Channel::MutualInformation() const {
  return (this->ShannonEntropyPrior() - this->ConditionalEntropyHyper());
}

double Channel::NormalizedMutualInformation() const {
  return this->MutualInformation() / sqrt(this->ShannonEntropyPrior()*this->ShannonEntropyOut());
}

double Channel::SymmetricUncertainty() const {
  return 2*this->MutualInformation() / (this->ShannonEntropyPrior() + this->ShannonEntropyOut());
}

} // namespace channel
