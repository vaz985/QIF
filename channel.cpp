#include "channel.h"
int main() {
  Channel c(2,3);
  std::cout << c << std::endl;
}

Channel::Channel(int n_in, int n_out) : n_in_(n_in), n_out_(n_out) {
  
  // The prior is a uniform distribution by default.
  this->prior_distribution = std::vector<double>(n_in, 1.0f/n_in);
  
  
  Randomize();
}

// This function parses a channel string.
void Channel::ParseInput(std::string input_str) {
  // TODO(thiagovas): Implement this function.
}

// This function returns a string that represents the
// current channel.
std::string Channel::to_string() {
  // TODO(thiagovas): Implement this function.
  //                  The string generated must be recognizable
  //                  by the ParseInput method.
  //                  The following piece of code must work:
  //                  ParseInput(channel.to_string());
}

// This function transposes the current channel.
// The input becomes the output, and the output becomes the
// input; matrix-wise we'll have p(x|y) instead of p(y|x).
void Channel::Transpose() {
  std::vector<double> p_y;
  this->h_matrix = this->c_matrix;
  for( int i=0; i<this->n_in_; i++ ) {
    for( int j=0; j<this->n_out_; j++ ) {
      this->h_matrix[i][j] = this->h_matrix[i][j] * this->prior_distribution[i];
    }
  }

  for( int i=0; i<this->n_out_; i++ ) {
    p_y.push_back(0.0);
    for( int j=0; j<this->n_in_; j++ ) {
      p_y[i] += this->h_matrix[j][i]; 
    }
    for( int j=0; j<this->n_in_; j++ ) {
      this->h_matrix[j][i] /= p_y[i];
    }
  } 
}

// This function randomizes the current channel.
// Maintaining the channel dimensions.
void Channel::Randomize() {
  // RNG
  std::mt19937 rng;
  rng.seed(std::random_device()());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0,100);
  // //
  int norm = 0;
  for( int i=0; i<this->n_in_; i++ ) {
    std::vector<double> row;
    for( int j=0; j<this->n_out_; j++ ) {
      int n = dist(rng);
      norm += n;
      row.push_back(n);
    }
    this->c_matrix.push_back(row);
  }  
  for( int i=0; i<this->n_in_; i++ ) {
    for( int j=0; j<this->n_out_; j++ ) {
      this->c_matrix[i][j] /= norm;
    }
  }  
}

bool Channel::CompatibleChannels(Channel c1, Channel c2) {
  if(c1.n_in_ == c2.n_in_ && c1.in_names_ == c2.in_names_)
    return true;
  return false;
}

std::ostream& operator<< (std::ostream& stream, const Channel& c) {
  for( int i=0; i<c.n_in_; i++ ) {
    for( int j=0; j<c.n_out_; j++ ) {
      stream << c.c_matrix[i][j] << " ";
    } 
    if( (i+1) != c.n_in_ ) 
      stream << std::endl;
  }
  return stream;
}
double Channel::ShannonEntropy() {
  double entropy = 0;
  for(int i=0; i<this->n_out_; i++){
    entropy += (this->prior_distribution[i]*log2(1/this->prior_distribution[i]));
  } 
  return entropy;
}
double Channel::ConditionalEntropy() {
  double entropy = 0;
  for(int j=0; j<this->n_out_; j++){
    double conditional_entropy_X = 0;
    for(int i=0; i<this->n_in_; i++){
      conditional_entropy_X += (this->h_matrix[i][j] * log2(1/this->h_matrix[i][j]));
    }
    entropy += (this->out_distribution[j] * conditional_entropy_X);
  }
  return entropy;
}
double Channel::JointEntropy() {
  double entropy = 0;
  std::vector<std::vector<double> > v = this->c_matrix;
  for(int i=0; i<this->n_in_; i++) {
    for(int j=0; j<this->n_out_; j++) {
      v[i][j] = v[i][j] * this->prior_distribution[i]; 
    }
  }
  for(int i=0; i<this->n_in_; i++) {
    for(int j=0; j<this->n_out_; j++) {
      entropy += (v[i][j]*log2(1/v[i][j]));
    }
  } 
  return entropy;
}
double Channel::GuessingEntropy() {
  double entropy = 0;
  std::vector<double> v = this->prior_distribution;
  sort(v.begin(),v.end());
  for(int i=1; i<=this->n_in_; i++)
    entropy += (i*v[i]);
  return entropy;
}
double Channel::MutualInformation() {
  return (this->ConditionalEntropy() - this->ShannonEntropy());
}