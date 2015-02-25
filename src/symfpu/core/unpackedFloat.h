/*
** Copyright (C) 2015 Martin Brain
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** unpackedFloat.h
**
** Martin Brain
** martin.brain@cs.ox.ac.uk
** 03/06/14
**
** The working representation of a floating-point number.  This is
** from the packed representation in a few ways:
**
**  1. Explicit flags for NaN, Inf and Zero.
**  2. Significand is biased.
**  3. Hidden bit is explicit.
**  4. Subnormals are normalised.
**
** This makes numbers more uniform and makes it easier to implement
** compact and efficient algorithms.
*/

#include "symfpu/utils/common.h"
#include "symfpu/utils/properties.h"

#include "symfpu/core/ite.h"
#include "symfpu/core/nondet.h"

// For debugging only
#include <iostream>

#ifndef SYMFPU_UNPACKED_FLOAT
#define SYMFPU_UNPACKED_FLOAT

namespace symfpu {

  template<class t>
    class unpackedFloat {
  public :
    // Typedef the names from the traits for convenience
    typedef typename t::bwt bwt;
    typedef typename t::fpt fpt;
    typedef typename t::prop prop;
    typedef typename t::sbv sbv;
    typedef typename t::ubv ubv;


  protected :
    // TODO : protect these again
  public :
    prop nan;
    prop inf;
    prop zero;

    prop sign;
    sbv exponent;
    ubv significand;
  protected :

    // It is possible, but hopefully as difficult as possible to create,
    // via the constructor an invalid unpacked float

    // A piecewise / literal constructor using fpclass
    enum fpclass { FPCLASS_NAN, FPCLASS_INF, FPCLASS_ZERO, FPCLASS_NUMBER };

    unpackedFloat (const fpclass c, const prop &s, const sbv &exp, const ubv &signif) : 
      nan(c == FPCLASS_NAN), inf(c == FPCLASS_INF), zero(c == FPCLASS_ZERO),
      sign(s), exponent(exp), significand(signif)
      {}


    // Should only be used by ite
    friend ite<prop, unpackedFloat<t> >;

    // TODO : See above -- this should only be used by ite
  public :
    unpackedFloat (const prop &iteNaN, const prop &iteInf, const prop &iteZero,
		   const prop &iteSign, const sbv &iteExponent, const ubv &iteSignificand) :
      nan(iteNaN), inf(iteInf), zero(iteZero),
      sign(iteSign), exponent(iteExponent), significand(iteSignificand)
      {}
  private :

    // Used for special values
    // However this will also be passed through the operations, thus
    // if it is also a valid normal number then it will make proving
    // invariants easier.  In this case it is the value 1.0.

    static sbv defaultExponent(const fpt &fmt) {
      return sbv::zero(unpackedFloat<t>::exponentWidth(fmt));
    }

    static ubv defaultSignificand(const fpt &fmt) {
      bwt significandWidth = unpackedFloat<t>::significandWidth(fmt);

      return ubv::one(significandWidth) << (significandWidth - 1);
    }



  public :
    unpackedFloat (const prop &s, const sbv &exp, const ubv &signif) : 
      nan(false), inf(false), zero(false),
      sign(s), exponent(exp), significand(signif)
      {}

    unpackedFloat (const unpackedFloat<t> &old) :
      nan(old.nan), inf(old.inf), zero(old.zero),
      sign(old.sign), exponent(old.exponent), significand(old.significand)
      {}

    // Copy and over-write sign
    unpackedFloat (const unpackedFloat<t> &old, const prop &s) : 
      nan(old.nan), inf(old.inf), zero(old.zero),
      sign(ITE(old.nan, old.sign, s)), exponent(old.exponent), significand(old.significand)
      {}

    // Need to separately assert that this is valid
    unpackedFloat (const nonDetMarkerType &, const fpt &fmt) :
      nan(NONDET), inf(NONDET), zero(NONDET), sign(NONDET), 
      exponent(NONDET,unpackedFloat<t>::exponentWidth(fmt)),
      significand(NONDET,unpackedFloat<t>::significandWidth(fmt))
      {}

    // Swap back-ends
      template <class s> friend class unpackedFloat;

    template <class s>
    unpackedFloat (const unpackedFloat<s> &old) :
      nan(old.nan), inf(old.inf), zero(old.zero),
      sign(old.sign), exponent(old.exponent), significand(old.significand)
      {}
      

    static unpackedFloat<t> makeZero(const fpt &fmt, const prop &s) {
      return unpackedFloat<t>(FPCLASS_ZERO, s, defaultExponent(fmt), defaultSignificand(fmt));
    }

    static unpackedFloat<t> makeInf(const fpt &fmt, const prop &s) {
      return unpackedFloat<t>(FPCLASS_INF, s, defaultExponent(fmt), defaultSignificand(fmt));
    }

    static unpackedFloat<t> makeNaN(const fpt &fmt) {
      return unpackedFloat<t>(FPCLASS_NAN, false, defaultExponent(fmt), defaultSignificand(fmt));
    }

    inline const prop & getNaN(void) const { return this->nan; }
    inline const prop & getInf(void) const { return this->inf; }
    inline const prop & getZero(void) const { return this->zero; }
    inline const prop & getSign(void) const { return this->sign; }
    inline const sbv & getExponent(void) const { return this->exponent; }
    inline const ubv & getSignificand(void) const { return this->significand; }

    //    inline unpackedFloat<t> changeSign(const prop &newSign) {
    //      return unpackedFloat<t>(*this, newSign);
    //    }




    // Get the number of bits in the unpacked format corresponding to a
    // given packed format.  These are the unpacked counter-parts of
    //  format.exponentWidth() and format.significandWidth()

    static bwt exponentWidth(const fpt &format) {

      // Note that there is one more exponent above 0 than there is
      // below.  This is the opposite of 2's compliment but this is not
      // a problem because the highest packed exponent corresponds to
      // inf and NaN and is thus does not need to be represented in the
      // unpacked format.
      // However we do need to increase it to allow subnormals (packed)
      // to be normalised.

      bwt width = format.exponentWidth();

      // Could be improved to remove overflow concerns
      uint64_t minimumExponent = ((1 << (width - 1)) - 2) + (format.significandWidth() - 1);

      // Increase width until even the smallest subnormal can be normalised
      while ((1UL << (width - 1)) < minimumExponent) {
	++width;
      }

      return width;
    }

    static bwt significandWidth(const fpt &format) {
      // Hidden bit is already included in the floating-point format
      return format.significandWidth();
    }




    // These should all evaluate to a literal value but are given as
    // sbv's to make their use easier and to avoid concerns of overflow.

    static sbv bias(const fpt &format) {
      sbv one(sbv::one(exponentWidth(format)));

      return (one << (format.exponentWidth() - 1)) - one;
    }

    static sbv maxNormalExponent(const fpt &format) {
      return bias(format);
    }

    static sbv minNormalExponent(const fpt &format) {
      return -(bias(format) - sbv::one(exponentWidth(format)));
    }

    static sbv maxSubnormalExponent(const fpt &format) {
      return -bias(format);
    }

    static sbv minSubnormalExponent(const fpt &format) {
      return maxSubnormalExponent(format) - sbv(exponentWidth(format),(significandWidth(format) - 2));
    } 

    inline prop inNormalRange(const fpt &format) const {
      return ((minNormalExponent(format) <= exponent) &&
	      (exponent <= maxNormalExponent(format)));
    }

    inline prop inSubnormalRange(const fpt &format) const {
      return ((minSubnormalExponent(format) <= exponent) &&
	      (exponent <= maxSubnormalExponent(format)));
    }

    inline sbv getSubnormalAmount(const fpt &format) const {
      return ITE(this->inSubnormalRange(format),
		 minNormalExponent(format) - exponent,
		 sbv::zero(exponent.getWidth()) );
    }

    inline prop isPositiveInf (void) const {
      return this->inf && !this->sign;
    }

    inline prop isNegativeInf (void) const {
      return this->inf && this->sign;
    }



    // Likewise, this is a convenience function
    static ubv leadingOne(const bwt sigWidth) {
      return ubv::one(sigWidth) << (sigWidth - 1);
    }

    static ubv nanPattern(const bwt sigWidth) {
      return ubv::one(sigWidth) << (sigWidth - 1); // For a qNaN, change for sNaN
    }



    unpackedFloat<t> extend (const bwt expExtension, const bwt sigExtension) const {
      return unpackedFloat<t>(this->nan, 
			      this->inf,
			      this->zero,
			      this->sign,
			      this->exponent.extend(expExtension),
			      this->significand.extend(sigExtension) << sigExtension);
    }


    // Moves the leading 1 up to the correct position, adjusting the
    // exponent as required.
    unpackedFloat<t> normaliseUp (const fpt &format) const {
      PRECONDITION(!(nan || inf || zero));  // Should not be attempting to normalise these.

      unpackedFloat<t> working(*this);
      bwt sigWidth = unpackedFloat<t>::significandWidth(format);
      bwt exWidth = unpackedFloat<t>::exponentWidth(format);
     
      for (bwt power = previousPowerOfTwo(sigWidth); power != 0; power >>= 1) {
	bwt rem = sigWidth - power;

	INVARIANT(rem > 0);

	ubv mask(ubv::allOnes(power).extend(rem) << rem);
	prop shiftNeeded((mask & working.significand).isAllZeros());

	// Has to be modular as in the case it is not needed,
	// performing the shift will loose information.
	working.significand = ITE(shiftNeeded, working.significand.modularLeftShift(power), working.significand);
	working.exponent = ITE(shiftNeeded, working.exponent - sbv(exWidth,power), working.exponent);
      }

      return working;
    }



    // Is a well formed unpacked struct of the given format?
    // The format is needed to ensure that subnormals are correct.
    // This invariant does not hold at all points in the code!
    prop valid(const fpt &format) const {
      bwt exWidth = exponentWidth(format);
      bwt sigWidth = significandWidth(format);

      PRECONDITION((exWidth == exponent.getWidth()) &&
		   (sigWidth == significand.getWidth()));


      prop hasLeadingOne(!(leadingOne(unpackedFloat<t>::significandWidth(format)) & significand).isAllZeros());



      // Subnormal numbers require an additional check to make sure they
      // do not have an unrepresentable amount of significand bits.
      sbv subnormalAmount(this->getSubnormalAmount(format));
      INVARIANT((sbv::zero(exWidth) <= subnormalAmount) &&
		(subnormalAmount <= sbv(exWidth,sigWidth)));

      // Invariant implies this following steps do not loose data
      ubv trimmedSubnormalAmount(subnormalAmount.toUnsigned().extract(positionOfLeadingOne(sigWidth),0));
      ubv mask(trimmedSubnormalAmount.orderEncode(sigWidth));

      prop correctlyAbbreviated((mask & significand).isAllZeros());



      prop normalCase   (!nan && !inf && !zero && inNormalRange(format)    && hasLeadingOne);
      prop subnormalCase(!nan && !inf && !zero && inSubnormalRange(format) && hasLeadingOne && correctlyAbbreviated);


    
      prop exponentIsDefault(defaultExponent(format) == exponent);
      prop significandIsDefault(defaultSignificand(format) == significand);

      prop NaNCase ( nan && !inf && !zero && exponentIsDefault && significandIsDefault && !sign);
      prop InfCase (!nan &&  inf && !zero && exponentIsDefault && significandIsDefault);
      prop ZeroCase(!nan && !inf &&  zero && exponentIsDefault && significandIsDefault);

      return (NaNCase || InfCase || ZeroCase || normalCase || subnormalCase);
    }

    // Just for debugging
    void print (void) const {
      std::cerr << "nan : " << this->nan << '\t'
		<< "inf : " << this->inf << '\t'
		<< "zero : " << this->zero << '\t'
		<< "sign : " << this->sign << '\t'
		<< "exponent : " << this->exponent << '\t'
		<< "significand : " << this->significand << std::endl;
    }

  };

template <class t>
  struct ite<typename t::prop, unpackedFloat<t> > {					
  static const unpackedFloat<t> iteOp (const typename t::prop &cond,		
			    const unpackedFloat<t> &l,					
			    const unpackedFloat<t> &r) {				
    return unpackedFloat<t>(ITE(cond, l.nan, r.nan),
			    ITE(cond, l.inf, r.inf),
			    ITE(cond, l.zero, r.zero),
			    ITE(cond, l.sign, r.sign),
			    ITE(cond, l.exponent, r.exponent),
			    ITE(cond, l.significand, r.significand));
    }
 };




};

#endif
