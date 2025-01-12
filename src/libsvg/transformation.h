/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <utility>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace libsvg {

class transformation
{
private:
  const std::string op;
  const std::string name;
protected:
  std::vector<double> args;

public:
  transformation(std::string op, std::string name) : op(std::move(op)), name(std::move(name)) { }
  virtual ~transformation() = default;

  [[nodiscard]] const std::string& get_op() const { return op; }
  [[nodiscard]] const std::string& get_name() const { return name; }
  [[nodiscard]] const std::string get_args() const;

  void add_arg(const std::string& arg);
  virtual std::vector<Eigen::Matrix3d> get_matrices() = 0;
};

class matrix : public transformation
{
public:
  matrix();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

class translate : public transformation
{
public:
  translate();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

class scale : public transformation
{
public:
  scale();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

class rotate : public transformation
{
public:
  rotate();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

class skew_x : public transformation
{
public:
  skew_x();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

class skew_y : public transformation
{
public:
  skew_y();

  std::vector<Eigen::Matrix3d> get_matrices() override;
};

} // namespace libsvg
