#ifndef LIBSVG_TRANSFORM_H
#define	LIBSVG_TRANSFORM_H

#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace libsvg {

class transformation {
private:
    const std::string op;
    const std::string name;
protected:
    std::vector<double> args;
    
public:
    transformation(const std::string op, const std::string name);
    virtual ~transformation();
    
    const std::string& get_op();
    const std::string& get_name();
    const std::string get_args();
    
    void add_arg(const std::string arg);
    virtual std::vector<Eigen::Matrix3d> get_matrices() = 0;
};

class matrix : public transformation {
public:
    matrix();
    ~matrix();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

class translate : public transformation {
public:
    translate();
    ~translate();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

class scale : public transformation {
public:
    scale();
    ~scale();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

class rotate : public transformation {
public:
    rotate();
    ~rotate();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

class skew_x : public transformation {
public:
    skew_x();
    ~skew_x();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

class skew_y : public transformation {
public:
    skew_y();
    ~skew_y();
    
    std::vector<Eigen::Matrix3d> get_matrices() override;
};

}

#endif	/* LIBSVG_TRANSFORM_H */
