#pragma once
#include <kalman/LinearizedMeasurementModel.hpp>
#include <kalman_lie/LieTypes.hpp>
#include <kalman_lie/NumericalDiff.hpp>

namespace Lie
{

/**
 * @brief Measurement model for measuring the position of the robot
 *        using two beacon-landmarks
 *
 * This is the measurement model for measuring the position of the robot.
 * The measurement is given by two landmarks in the space, whose positions are known.
 * The robot can measure the direct distance to both the landmarks, for instance
 * through visual localization techniques.
 *
 * @param T Numeric scalar type
 * @param CovarianceBase Class template to determine the covariance representation
 *                       (as covariance matrix (StandardBase) or as lower-triangular
 *                       coveriace square root (SquareRootBase))
 */
template<typename T, template<class> class CovarianceBase = Kalman::StandardBase>
class LiePositionMeasurementModel : public Kalman::LinearizedMeasurementModel<State<T>, LieMeasurement<T>, CovarianceBase>
{
public:
    //! State type shortcut definition
    typedef State<T> S;

    //! Measurement type shortcut definition
    typedef LieMeasurement<T> M;

    // Wraps the system model cost function in an automatic differentiation functor
    template <typename Parent>
    struct LieFunctor_ : Functor<T>
    {
        Parent* m;

        LieFunctor_(Parent* m) : m(m)
        {
        }

        int operator()(const Eigen::VectorXd& x, Eigen::VectorXd& fvec) const
        {
            S xx;
            xx.x = x;
            xx.v.setZero();
            // Cost function
            fvec = m->h(xx);
            return 0;
        }

        int inputs() const { return 6; }  // There are two parameters of the model
        int values() const { return 6; }  // The number of observations
    };
    using LieFunctor = LieFunctor_<LiePositionMeasurementModel<T>>;
    NumericalDiffFunctor<LieFunctor> num_diff;

    LiePositionMeasurementModel() : num_diff(this)
    {
        // Setup noise jacobian. As this one is static, we can define it once
        // and do not need to update it dynamically
        this->V.setIdentity();
    }

    /**
     * @brief Definition of (possibly non-linear) measurement function
     *
     * This function maps the system state to the measurement that is expected
     * to be received from the sensor assuming the system is currently in the
     * estimated state.
     *
     * @param [in] x The system state in current time-step
     * @returns The (predicted) sensor measurement for the system state
     */
    M h(const S& x) const override
    {
        M measurement;
        measurement = x.x;
        return measurement;
    }

protected:

    /**
     * @brief Update jacobian matrices for the system state transition function using current state
     *
     * This will re-compute the (state-dependent) elements of the jacobian matrices
     * to linearize the non-linear measurement function \f$h(x)\f$ around the
     * current state \f$x\f$.
     *
     * @note This is only needed when implementing a LinearizedSystemModel,
     *       for usage with an ExtendedKalmanFilter or SquareRootExtendedKalmanFilter.
     *       When using a fully non-linear filter such as the UnscentedKalmanFilter
     *       or its square-root form then this is not needed.
     *
     * @param x The current system state around which to linearize
     * @param u The current system control input
     */
    void updateJacobians( const S& x )
    {
        // H = dh/dx (Jacobian of measurement function w.r.t. the state)
        this->H.setZero();
        std::cout << this->H.rows() << ", " << this->H.cols() << std::endl;
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> jac(6,12);
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> xx = x.x;
        num_diff.df(xx, jac);
        this->H = jac;
    }
};

} // namespace Robot