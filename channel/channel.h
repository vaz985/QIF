#ifndef _channel_channel_h
#define _channel_channel_h
#include <string>
#include <vector>
#include <set>
#include <map>

#include <iostream>

namespace channel {

class Channel {
  public:
    Channel(int n_in=2, int n_out=2);
  
    // Random with specific prior distribution
    Channel(std::vector<double> prior_distribution, int n_in=2, int n_out=2);

    Channel(const std::vector<std::vector<double> >& c_matrix,
            int base_norm=0);

    Channel(const std::vector<std::vector<double> > & c_matrix,
            std::vector<double> prior_distribution,
            int base_norm=0);

    std::string cname() const {
      return this->cname_;
    }


    void set_cname(std::string cname) {
      this->cname_ = cname;
    }

    int n_in() const {
      return this->n_in_;
    }

    int n_out() const {
      return this->n_out_;
    }

    void set_in_names(std::vector<std::string> in_names) {
      this->in_names_ = in_names;
    }

    void set_out_names(std::vector<std::string> out_names) {
      this->out_names_ = out_names;
    }

    std::vector<std::string> in_names() const {
      return this->in_names_;
    }

    std::vector<std::string> out_names() const {
      return this->out_names_;
    }

    const std::vector<double>& prior_distribution() const {
      return this->prior_distribution_;
    }

    const std::vector<double>& out_distribution() const {
      return this->out_distribution_;
    }

    const std::vector<double>& max_pinput() const {
      return this->max_pinput_;
    }

    const std::vector<double>& max_poutput() const {
      return this->max_poutput_;
    }

    const std::map<std::string,int>& pos_in_names() const {
      return this->pos_in_names_;
    }

    const std::map<std::string,int>& pos_out_names() const {
      return this->pos_out_names_;
    }

    const std::vector<std::vector<double> >& c_matrix() const {
        return this->c_matrix_;
    }

    const std::vector<std::vector<double> >& j_matrix() const {
        return this->j_matrix_;
    }
    
    const std::vector<std::vector<double> >& h_matrix() const {
        return this->h_matrix_;
    }

    const int in_index(std::string s) const {
        return this->pos_in_names_.at(s);
    }

    const int out_index(std::string s) const {
        return this->pos_out_names_.at(s);
    }

    void insert_in_index(std::string s, int i) {
        this->pos_in_names_[s] = i;
    }

    void insert_out_index(std::string s, int i) {
        this->pos_out_names_[s] = i;
    }

    // This function parses a channel string.
    void ParseInput(std::string input_str);

    // This function parses a channel file.
    void ParseFile(std::string fname);

    // This function resets the class to an initial state.
    void Reset();

    // Make identity channel
    void Identity();

    // This function returns a string that represents the
    // current channel.
    std::string to_string() const;

    // Two channels are compatible if they have the same input set.
    // This function checks that.
    static bool CompatibleChannels(const Channel& c1, const Channel& c2);


    friend std::ostream& operator<< (std::ostream& stream, const Channel& c);

    friend Channel operator|| (const Channel& c1, const Channel& c2);

    friend Channel operator* (const Channel& c1, const Channel& c2);

    static Channel hidden_choice (const Channel& c1, const double prob, 
                                  const Channel& c2);

    static Channel visible_choice (const Channel& c1, const double prob, 
                                   const Channel& c2);

    static Channel visible_conditional (const Channel& c1, 
                                        std::vector<std::string> &A, 
                                        const Channel& c2);
    
    static Channel hidden_conditional (const Channel& c1,
                                        std::vector<std::string> &A,
                                        const Channel& c2);

    // Upper n Lower Bounds
    static std::pair<double, double> 
      parallel_vulnerability (const Channel& c1, const Channel& c2,
                              std::vector<double> prior,
                              std::vector<std::vector<double>> &g);

    // Linear Bounds
    static double 
      visible_choice_vulnerability (const Channel& c1,
                                    const Channel& c2,
                                    const double prob,
                                    std::vector<std::vector<double>> &g);

    static std::pair<double, double> 
      hidden_choice_vulnerability (const Channel& c1,
                                   const Channel& c2,
                                   const double prob,
                                   std::vector<std::vector<double>> &g);

    static double 
      visible_conditional_vulnerability (const Channel& c1,
                                         const Channel& c2,
                                         const std::vector<std::string> &A);
    static std::pair<double, double>
      hidden_conditional_vulnerability (const Channel& c1,
                                        const Channel& c2,
                                        const std::vector<std::string> &A,
                                        const std::vector<double> &prior,
                                        const std::vector<std::vector<double>> &g);

    double ShannonEntropyPrior() const;
    double ShannonEntropyOut() const;
    double ConditionalEntropy() const;
    double ConditionalEntropyHyper() const;
    double JointEntropy() const;
    double GuessingEntropy() const;
    double MutualInformation() const;
    double NormalizedMutualInformation() const;
    double SymmetricUncertainty() const;
    //double PriorGVun() const; // G_ID
		double PriorGVun(std::vector<std::vector<double> > g) const;
    //double PostGVun() const; // G_ID
		double PostGVun(const std::vector<std::vector<double> > &g) const;
		double PostGVun(const std::vector<double> &prior_distribution, 
                    const std::vector<std::vector<double> > &g) const;

    // Maps input/output names to their index
    std::map<std::string, int> pos_in_names_, pos_out_names_;
  private:
    // Channel Name
    std::string cname_ = "";

    // This is the channel matrix. ( p(y|x) )
    std::vector<std::vector<double> > c_matrix_;

    // This is the posterior probability matrix. ( hyper distribution p(x|y) )
    std::vector<std::vector<double> > h_matrix_;

    // This is the prior distribution.
    std::vector<double> prior_distribution_;

    // This is the output distribution.
    std::vector<double> out_distribution_;

    // This is the joint matrix. ( p(x,y) )
    std::vector<std::vector<double> > j_matrix_;

    // The maximum p(x, y) per x; used by the bayes' metrics.
    std::vector<double> max_pinput_;

    // The maximum p(x, y) per y; used by the bayes' metrics.
    std::vector<double> max_poutput_;

    // These ints keep the number of input lines we have,
    // and the number of output lines.
    int n_in_, n_out_;

    // This is the norm used to randomly generate the channel.
    int base_norm_;

    // These vectors keep the names of each input line and
    // each output line
    std::vector<std::string> in_names_, out_names_;

    // This function randomizes the current channel.
    // Maintaining the channel dimensions.
    void Randomize();

    void build_channel(std::vector<std::vector<double> > c_matrix);

    void build_channel(std::vector<std::vector<double> > c_matrix,
                        std::vector<double> prior_distribution);

    void setup_default_names();
    void setup_in_out_map();
};

} // namespace channel

#endif
