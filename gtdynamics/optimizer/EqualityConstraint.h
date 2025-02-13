/* ----------------------------------------------------------------------------
 * GTDynamics Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file  EqualityConstraint.h
 * @brief Equality constraints in constrained optimization.
 * @author: Yetong Zhang, Frank Dellaert
 */

#pragma once

#include <gtsam/nonlinear/ExpressionFactor.h>
#include <gtsam/nonlinear/NonlinearFactor.h>

namespace gtdynamics {

/**
 * Equality constraint base class.
 */
class EqualityConstraint {
 public:
  typedef EqualityConstraint This;
  typedef boost::shared_ptr<This> shared_ptr;

  /** Default constructor. */
  EqualityConstraint() {}

  /** Destructor. */
  virtual ~EqualityConstraint() {}

  /**
   * @brief Create a factor representing the component in the merit function.
   *
   * @param mu penalty parameter.
   * @param bias additional bias.
   * @return a factor representing 1/2 mu||g(x)+bias||_Diag(tolerance^2)^2.
   */
  virtual gtsam::NoiseModelFactor::shared_ptr createFactor(
      const double mu,
      boost::optional<gtsam::Vector&> bias = boost::none) const = 0;

  /**
   * @brief Check if constraint violation is within tolerance.
   *
   * @param x values to evalute constraint at.
   * @return bool representing if is feasible.
   */
  virtual bool feasible(const gtsam::Values& x) const = 0;

  /**
   * @brief Evaluate the constraint violation, g(x).
   *
   * @param x values to evalute constraint at.
   * @return a vector representing the constraint violation in each dimension.
   */
  virtual gtsam::Vector operator()(const gtsam::Values& x) const = 0;

  /** @brief Constraint violation scaled by tolerance, e.g. g(x)/tolerance. */
  virtual gtsam::Vector toleranceScaledViolation(
      const gtsam::Values& x) const = 0;

  /** @brief return the dimension of the constraint. */
  virtual size_t dim() const = 0;

  /// Return keys of variables involved in the constraint.
  virtual std::set<gtsam::Key> keys() const{
    return std::set<gtsam::Key>();
  }
};

/** Equality constraint that force g(x) = 0, where g(x) is a scalar-valued
 * function. */
class DoubleExpressionEquality : public EqualityConstraint {
 public:
 protected:
  gtsam::Expression<double> expression_;
  double tolerance_;

 public:
  /**
   * @brief Constructor.
   *
   * @param expression  expression representing g(x).
   * @param tolerance   scalar representing tolerance.
   */
  DoubleExpressionEquality(const gtsam::Expression<double>& expression,
                           const double& tolerance)
      : expression_(expression), tolerance_(tolerance) {}

  gtsam::NoiseModelFactor::shared_ptr createFactor(
      const double mu,
      boost::optional<gtsam::Vector&> bias = boost::none) const override;

  bool feasible(const gtsam::Values& x) const override;

  gtsam::Vector operator()(const gtsam::Values& x) const override;

  gtsam::Vector toleranceScaledViolation(const gtsam::Values& x) const override;

  size_t dim() const override { return 1; }

  std::set<gtsam::Key> keys() const override{
    return expression_.keys();
  }
};

/** Equality constraint that force g(x) = 0, where g(x) is a vector-valued
 * function. */
template <int P>
class VectorExpressionEquality : public EqualityConstraint {
 public:
  using VectorP = Eigen::Matrix<double, P, 1>;

 protected:
  gtsam::Expression<VectorP> expression_;
  VectorP tolerance_;

 public:
  /**
   * @brief Constructor.
   *
   * @param expression  expression representing g(x).
   * @param tolerance   vector representing tolerance in each dimension.
   */
  VectorExpressionEquality(const gtsam::Expression<VectorP>& expression,
                           const VectorP& tolerance)
      : expression_(expression), tolerance_(tolerance) {}

  gtsam::NoiseModelFactor::shared_ptr createFactor(
      const double mu,
      boost::optional<gtsam::Vector&> bias = boost::none) const override;

  bool feasible(const gtsam::Values& x) const override;

  gtsam::Vector operator()(const gtsam::Values& x) const override;

  gtsam::Vector toleranceScaledViolation(const gtsam::Values& x) const override;

  size_t dim() const override;

  std::set<gtsam::Key> keys() const override{
    return expression_.keys();
  }
};

/// Container of EqualityConstraint.
class EqualityConstraints : public std::vector<EqualityConstraint::shared_ptr> {
 private:
  using Base = std::vector<EqualityConstraint::shared_ptr>;

  template <typename DERIVEDCONSTRAINT>
  using IsDerived = typename std::enable_if<
      std::is_base_of<EqualityConstraint, DERIVEDCONSTRAINT>::value>::type;

 public:
  EqualityConstraints() : Base() {}

  /// Add a set of equality constraints.
  void add (const EqualityConstraints& other) {
    insert(end(), other.begin(), other.end());
  }

  /// Emplace a shared pointer to constraint of given type.
  template <class DERIVEDCONSTRAINT, class... Args>
  IsDerived<DERIVEDCONSTRAINT> emplace_shared(Args&&... args) {
    push_back(boost::allocate_shared<DERIVEDCONSTRAINT>(
        Eigen::aligned_allocator<DERIVEDCONSTRAINT>(),
        std::forward<Args>(args)...));
  }

};

}  // namespace gtdynamics

#include <gtdynamics/optimizer/EqualityConstraint-inl.h>
