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
    
    virtual const std::string& get_op();
    virtual const std::string& get_name();
    virtual const std::string get_args();
    
    virtual void add_arg(const std::string arg);
    virtual std::vector<Eigen::Matrix3d> get_matrices() = 0;
};

class matrix : public transformation {
public:
    matrix();
    virtual ~matrix();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

class translate : public transformation {
public:
    translate();
    virtual ~translate();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

class scale : public transformation {
public:
    scale();
    virtual ~scale();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

class rotate : public transformation {
public:
    rotate();
    virtual ~rotate();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

class skew_x : public transformation {
public:
    skew_x();
    virtual ~skew_x();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

class skew_y : public transformation {
public:
    skew_y();
    virtual ~skew_y();
    
    virtual std::vector<Eigen::Matrix3d> get_matrices();
};

}

#endif	/* LIBSVG_TRANSFORM_H */
