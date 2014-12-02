#ifndef DGY_TRANSFORMER_DOT_H
#define DGY_TRANSFORMER_DOT_H

#include <SgtCore/Branch.h>

namespace SmartGridToolbox
{
   /// @brief Delta-grounded wye transformer.
   ///
   /// Note that a 1:1 turns ratio will not result in a 1:1 voltage ratio for this transformer connection.
   /// This is because the voltage across the windings on the primary (Delta) side is the phase-phase voltage, so
   /// a 1:1 turns ratio will result in a @f$1:\sqrt(3)@f$ voltage ratio, where all voltages are line-ground. It is
   /// therefore important that the parameter to the constructor is the turns ratio, not the voltage ratio.
   class DgyTransformer : public BranchAbc
   {
      public:

      /// @name Lifecycle
      /// @{

         /// @brief Constructor
         /// @param a The complex turns ratio (not voltage ratio) for each of the six windings.
         /// @param ZL The leakage impedance, must be > 0.
         DgyTransformer(const std::string& id, Complex nomVRatioDY, Complex offNomRatioDY, Complex ZL) :
            BranchAbc(id, Phase::A | Phase::B | Phase::C, Phase::A | Phase::B | Phase::C),
            nomVRatioDY_(nomVRatioDY), offNomRatioDY_(offNomRatioDY), YL_(1.0/ZL)
         {
            // Empty.
         }

      /// @}

      /// @name Component Type:
      /// @{

         static constexpr const char* sComponentType()
         {
            return "dgy_transformer";
         }

         virtual const char* componentType() const override
         {
            return sComponentType();
         }

      /// @}

      /// @name Parameters:
      /// @{

         Complex nomVRatioDY() const
         {
            return nomVRatioDY_;
         }
         
         void setNomVRatioDY(Complex nomVRatioDY)
         {
            nomVRatioDY_ = nomVRatioDY;
         }

         Complex offNomRatioDY() const
         {
            return offNomRatioDY_;
         }
 
         void setOffNomRatioDY(Complex offNomRatioDY)
         {
            offNomRatioDY_ = offNomRatioDY;
         }
 
         Complex a() const
         {
            return offNomRatioDY_ * nomVRatioDY_;  
         }

         Complex ZL() const
         {
            return 1.0 / YL_;
         }

         void setZL(Complex ZL)
         {
            YL_ = 1.0 / ZL;
         }

      /// @}

      /// @name Overridden from BranchAbc:
      /// @{

         virtual const arma::Mat<Complex> Y() const override;

      /// @}

      private:

         Complex nomVRatioDY_; ///< Nominal voltage ratio, V_D / V_Y where V_D is phase-phase and V_Y is phase-ground.
         Complex offNomRatioDY_; ///< Off nominal complex turns ratio.
         Complex YL_; ///< Series leakage admittance.
   };
}

#endif // DGY_TRANSFORMER_DOT_H
