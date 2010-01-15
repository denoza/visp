/****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 1998-2010 Inria. All rights reserved.
 *
 * This software was developed at:
 * IRISA/INRIA Rennes
 * Projet Lagadic
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * http://www.irisa.fr/lagadic
 *
 * This file is part of the ViSP toolkit.
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * LICENSE included in the packaging of this file.
 *
 * Licensees holding valid ViSP Professional Edition licenses may
 * use this file in accordance with the ViSP Commercial License
 * Agreement provided with the Software.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Contact visp@irisa.fr if any conditions of this licensing are
 * not clear to you.
 *
 * Description:
 * Moving edges.
 *
 * Authors:
 * Eric Marchand
 *
 *****************************************************************************/



#include <visp/vpMeEllipse.h>

#include <visp/vpMe.h>
#include <visp/vpRobust.h>
#include <visp/vpTrackingException.h>
#include <visp/vpDebug.h>
#include <visp/vpImagePoint.h>


/*!
  Computes the \f$ \theta \f$ angle which represents the angle between the tangente to the curve and the i axis.

  \param theta : The computed value.
  \param K : The parameters of the ellipse.
  \param iP : the point belonging th the ellipse where the angle is computed.
*/
void
computeTheta(double &theta, vpColVector &K, vpImagePoint iP)
{

  double i = iP.get_i();
  double j = iP.get_j();

  double A = 2*i+2*K[1]*j + 2*K[2] ;
  double B = 2*K[0]*j + 2*K[1]*i + 2*K[3];

  theta = atan2(A,B) ; //Angle between the tangente and the i axis.

  while (theta > M_PI) { theta -= M_PI ; }
  while (theta < 0) { theta += M_PI ; }
}


/*!
  Basic constructor that calls the constructor of the class vpMeTracker.
*/
vpMeEllipse::vpMeEllipse():vpMeTracker()
{
  vpCDEBUG(1) << "begin vpMeEllipse::vpMeEllipse() " <<  std::endl ;

  // redimensionnement du vecteur de parametre
  // i^2 + K0 j^2 + 2 K1 i j + 2 K2 i + 2 K3 j + K4

  circle = false ;
  K.resize(5) ;

  alpha1 = 0 ;
  alpha2 = 2*M_PI ;

  //j1 = j2 = i1 = i2 = 0 ;
  iP1.set_i(0);
  iP1.set_j(0);
  iP2.set_i(0);
  iP2.set_j(0);
  
  m00 = m01 = m10 = m11 = m20 = m02 = mu11 = mu20 = mu02 = 0;

  vpCDEBUG(1) << "end vpMeEllipse::vpMeEllipse() " << std::endl ;
}


/*!
  Basic destructor.
*/
vpMeEllipse::~vpMeEllipse()
{
  vpCDEBUG(1) << "begin vpMeEllipse::~vpMeEllipse() " << std::endl ;

  list.kill();
  angle.kill();

  vpCDEBUG(1) << "end vpMeEllipse::~vpMeEllipse() " << std::endl ;
}


  /*!
    Construct a list of vpMeSite moving edges at a particular sampling
    step between the two extremities. The two extremities are defined by
    the points with the smallest and the biggest \f$ alpha \f$ angle.

    \param I : Image in which the ellipse appears.
  */
void
vpMeEllipse::sample(vpImage<unsigned char> & I)
{
  vpCDEBUG(1) <<"begin vpMeEllipse::sample() : "<<std::endl ;

  unsigned height = I.getHeight() ;
  unsigned width = I.getWidth() ;

  double n_sample;

  if (me->sample_step==0)
  {
    std::cout << "In vpMeEllipse::sample: " ;
    std::cout << "function called with sample step = 0" ;
    //return fatalError ;
  }

  double j, i;//, j11, i11;
  vpImagePoint iP11;
  j = i = 0.0 ;

  double incr = vpMath::rad(me->sample_step) ; // angle increment en degree
  vpColor col = vpColor::red ;
  getParameters() ;


  // Delete old list
  list.front();
  list.kill();

  angle.front();
  angle.kill();

  // sample positions

  double k = alpha1 ;
  while (k<alpha2)
  {

//     j = a *cos(k) ; // equation of an ellipse
//     i = b *sin(k) ; // equation of an ellipse

    j = a *sin(k) ; // equation of an ellipse
    i = b *cos(k) ; // equation of an ellipse

    // (i,j) are the coordinates on the origin centered ellipse ;
    // a rotation by "e" and a translation by (xci,jc) are done
    // to get the coordinates of the point on the shifted ellipse
//     iP11.set_j( iPc.get_j() + ce *j - se *i );
//     iP11.set_i( iPc.get_i() -( se *j + ce *i) );

    iP11.set_j( iPc.get_j() + ce *j + se *i );
    iP11.set_i( iPc.get_i() - se *j + ce *i );

    vpDisplay::displayCross(I, iP11,  5, col) ;

    double theta ;
    computeTheta(theta, K, iP11)  ;

    // If point is in the image, add to the sample list
    if(!outOfImage(vpMath::round(iP11.get_i()), vpMath::round(iP11.get_j()), 0, height, width))
    {
      vpMeSite pix ;
      pix.init((int)iP11.get_i(), (int)iP11.get_j(), theta) ;
      pix.setDisplay(selectDisplay) ;
      pix.suppress = 0 ;

      if(vpDEBUG_ENABLE(3))
      {
	vpDisplay::displayCross(I,iP11, 5, vpColor::blue);
      }
      list.addRight(pix);
      angle.addRight(k);
    }
    k += incr ;

  }
  vpMeTracker::initTracking(I) ;

  n_sample = list.nbElements() ;

  vpCDEBUG(1) << "end vpMeEllipse::sample() : " ;
  vpCDEBUG(1) << n_sample << " point inserted in the list " << std::endl  ;
}


/*!
	
  Resample the ellipse if the number of sample is less than 90% of the
  expected value.
	
  \note The expected value is computed thanks to the difference between the smallest and the biggest \f$ \alpha \f$ angles
  and the parameter which indicates the number of degrees between
  two points (vpMe::sample_step).

  \param I : Image in which the ellipse appears.
*/
void
vpMeEllipse::reSample(vpImage<unsigned char>  &I)
{
  int n = numberOfSignal() ;
  double expecteddensity = (alpha2-alpha1) / vpMath::rad((double)me->sample_step);
  if ((double)n<0.9*expecteddensity)
    sample(I) ;
}


/*!
  Computes the length of the semiminor axis \f$ a \f$, the length of the semimajor axis \f$ b \f$ and, 
  \f$ e \f$ which is the angle made by the major axis and the i axis of the image frame \f$ (i,j) \f$.
  
  All those computations are made thanks to the parameters \f$ K = {K_0, ..., K_4} \f$.
*/
void
vpMeEllipse::getParameters()
{

  double k[6] ;
  for (int i=0 ; i < 5 ; i++)
    k[i+1] = K[i] ;
  k[0] = 1 ;

  double d = k[2]*k[2] - k[0]*k[1];


  iPc.set_i( (k[1] * k[3] - k[2] * k[4]) / d );
  iPc.set_j( (k[0] * k[4] - k[2] * k[3]) / d );

  double sq =  sqrt(vpMath::sqr(k[1]-k[0]) + 4.0*vpMath::sqr(k[2])) ;
  if (circle ==true)
  {
    e = 0 ;
  }
  else
  {
    e = (k[1] - k[0] + sq) / (2.0*k[2]);
    e = (-1/e) ;

    e = atan(e) ;
  }

  if(e < 0.0)  e += M_PI ;

  ce = cos(e) ;
  se = sin(e) ;

  double num = 2.0*(k[0]*iPc.get_i()*iPc.get_i() + 2.0*k[2]*iPc.get_j()*iPc.get_i() + k[1]*iPc.get_j()*iPc.get_j() - k[5]) ;
  double a2 = num / (k[0] + k[1] + sq ) ;
  double b2 = num / (k[0] + k[1] - sq ) ;

  a = sqrt( a2 ) ;
  b = sqrt( b2 ) ;

}

/*!
  Print the parameters \f$ K = {K_0, ..., K_4} \f$ and the coordinates of the ellipse center.
*/
void
vpMeEllipse::printParameters()
{
  std::cout << "K" << std::endl ;
  std::cout << K.t() ;
  std::cout << iPc << std::endl ;
}

/*!
  Computes the \f$ alpha \f$ angle of the two points and store them into alpha1 for the smallest and alpha2 for the biggest.

  \note this function is usefull only during the initialization.

  \param pt1 : First point whose \f$ alpha \f$ angle is computed.
  \param pt2 : Second point whose \f$ alpha \f$ angle is computed.
*/ 
void
vpMeEllipse::computeAngle(vpImagePoint pt1, vpImagePoint pt2)
{

  getParameters() ;
  double j1, i1, j11, i11;
  j1 =  i1 =  0.0 ;

  int number_of_points = 2000 ;
  double incr = 2 * M_PI / number_of_points ; // angle increment

  double dmin1 = 1e6  ;
  double dmin2 = 1e6  ;

  double k =  -M_PI ;
  while(k < M_PI) {

//     j1 = a *cos(k) ; // equation of an ellipse
//     i1 = b *sin(k) ; // equation of an ellipse

    j1 = a *sin(k) ; // equation of an ellipse
    i1 = b *cos(k) ; // equation of an ellipse

    // (i1,j1) are the coordinates on the origin centered ellipse ;
    // a rotation by "e" and a translation by (xci,jc) are done
    // to get the coordinates of the point on the shifted ellipse
//     j11 = iPc.get_j() + ce *j1 - se *i1 ;
//     i11 = iPc.get_i() -( se *j1 + ce *i1) ;

    j11 = iPc.get_j() + ce *j1 + se *i1 ;
    i11 = iPc.get_i() - se *j1 + ce *i1 ;

    double  d = vpMath::sqr(pt1.get_i()-i11) + vpMath::sqr(pt1.get_j()-j11) ;
    if (d < dmin1)
    {
      dmin1 = d ;
      alpha1 = k ;
    }
    d = vpMath::sqr(pt2.get_i()-i11) + vpMath::sqr(pt2.get_j()-j11) ;
    if (d < dmin2)
    {
      dmin2 = d ;
      alpha2 = k ;
    }
    k += incr ;
  }

  if (alpha2 <alpha1) alpha2 += 2*M_PI ;

  vpCDEBUG(1) << "end vpMeEllipse::computeAngle(..)" << alpha1 << "  " << alpha2 << std::endl ;
}


/*!
  Compute the \f$ theta \f$ angle for each vpMeSite.

  \note The \f$ theta \f$ angle is usefull during the tracking part.
*/
void
vpMeEllipse::updateTheta()
{
  vpMeSite p;
  list.front();
  double theta;
  for (int i=0 ; i < list.nbElement() ; i++)
  {
    p = list.value() ;
    vpImagePoint iP;
    iP.set_i(p.ifloat);
    iP.set_j(p.jfloat);
    computeTheta(theta, K, iP) ;
    p.alpha = theta ;
    list.modify(p) ;
    list.next() ;
  }
}

/*!
  Suppress the vpMeSite which are no more detected as point which belongs to the ellipse edge.
*/
void
vpMeEllipse::suppressPoints()
{
  // Loop through list of sites to track
  list.front();
  angle.front();
  while(!list.outside())
  {
    vpMeSite s = list.value() ;//current reference pixel
    if (s.suppress != 0)
    {
      list.suppress() ;
      angle.suppress();
    }
    else
    {
      list.next() ;
      angle.next();
    }
  }
}


/*!  
  Seek along the ellipse edge defined by its equation, the two extremities of
  the ellipse (ie the two points with the smallest and the biggest \f$ \alpha \f$ angle.

  \param I : Image in which the ellipse appears.
*/
void
vpMeEllipse::seekExtremities(vpImage<unsigned char>  &I)
{
  int rows = I.getHeight() ;
  int cols = I.getWidth() ;

  vpImagePoint ip;

  int  memory_range = me->range ;
  me->range = 2 ;

  double  memory_mu1 = me->mu1 ;
  me->mu1 = 0.5 ;

  double  memory_mu2 = me->mu2 ;
  me->mu2 = 0.5 ;

  double incr = vpMath::rad(2.0) ;

  if (alpha2-alpha1 < 2*M_PI-vpMath::rad(6.0))
  {
    vpMeSite P;
    double k = alpha1;
    double i1,j1;

    for (int i=0 ; i < 3 ; i++)
    {
      k -= incr;
      //while ( k < -M_PI ) { k+=2*M_PI; }

      i1 = b *cos(k) ; // equation of an ellipse
      j1 = a *sin(k) ; // equation of an ellipse
      P.ifloat = iPc.get_i() - se *j1 + ce *i1 ; P.i = (int)P.ifloat ;
      P.jfloat = iPc.get_j() + ce *j1 + se *i1 ; P.j = (int)P.jfloat ;

      if(!outOfImage(P.i, P.j, 5, rows, cols))
      {
        P.track(I,me,false) ;

        if (P.suppress ==0)
        {
          list += P ;
          angle += k;
          if (vpDEBUG_ENABLE(3)) {
            ip.set_i( P.i );
            ip.set_j( P.j );

            vpDisplay::displayCross(I, ip, 5, vpColor::green) ;
          }
        }
        else {
	  if (vpDEBUG_ENABLE(3)) {
	    ip.set_i( P.i );
	    ip.set_j( P.j );
	    vpDisplay::displayCross(I, ip, 10, vpColor::blue) ;
	  }
        }
      }
    }

    k = alpha2;

    for (int i=0 ; i < 3 ; i++)
    {
      k += incr;
      //while ( k > M_PI ) { k-=2*M_PI; }

      i1 = b *cos(k) ; // equation of an ellipse
      j1 = a *sin(k) ; // equation of an ellipse
      P.ifloat = iPc.get_i() - se *j1 + ce *i1 ; P.i = (int)P.ifloat ;
      P.jfloat = iPc.get_j() + ce *j1 + se *i1 ; P.j = (int)P.jfloat ;

      if(!outOfImage(P.i, P.j, 5, rows, cols))
      {
        P.track(I,me,false) ;

        if (P.suppress ==0)
        {
	  list += P ;
          angle += k;
	  if (vpDEBUG_ENABLE(3)) {
	    ip.set_i( P.i );
	    ip.set_j( P.j );

	    vpDisplay::displayCross(I, ip, 5, vpColor::green) ;
	  }
        }
        else {
	  if (vpDEBUG_ENABLE(3)) {
	    ip.set_i( P.i );
	    ip.set_j( P.j );
	    vpDisplay::displayCross(I, ip, 10, vpColor::blue) ;
	  }
        }
      }
    }
  }

  suppressPoints() ;

  me->range = memory_range ;
  me->mu1 = memory_mu1 ;
  me->mu2 = memory_mu2 ;
}


/*!
  Finds in the list of vpMeSite the two points with the smallest and the biggest \f$ \alpha \f$ angle value, and stores them.
*/
void
vpMeEllipse::setExtremities()
{
  double alphamin = +1e6;
  double alphamax = -1e6;
  double imin = 0;
  double jmin = 0;
  double imax = 0;
  double jmax = 0;

  // Loop through list of sites to track
  list.front();
  angle.front();

  while(!list.outside())
  {
    vpMeSite s = list.value() ;//current reference pixel
    double alpha = angle.value();
    if (alpha < alphamin)
    {
      alphamin = alpha;
      imin = s.ifloat ;
      jmin = s.jfloat ;
    }

    if (alpha > alphamax)
    {
      alphamax = alpha;
      imax = s.ifloat ;
      jmax = s.jfloat ;
    }
    list.next() ;
    angle.next();
  }

  alpha1 = alphamin;
  alpha2 = alphamax;
  iP1.set_ij(imin,jmin);
  iP2.set_ij(imax,jmax);
}


/*!
  Least squares method used to make the tracking more robust. It
  ensures that the points taken into account to compute the right
  equation belong to the ellipse.
*/
void
vpMeEllipse::leastSquare()
{
  // Construction du systeme Ax=b
  //! i^2 + K0 j^2 + 2 K1 i j + 2 K2 i + 2 K3 j + K4
  // A = (j^2 2ij 2i 2j 1)   x = (K0 K1 K2 K3 K4)^T  b = (-i^2 )
  int i ;

  vpMeSite p ;

  int iter =0 ;
  vpColVector b(numberOfSignal()) ;
  vpRobust r(numberOfSignal()) ;
  r.setThreshold(2);
  r.setIteration(0) ;
  vpMatrix D(numberOfSignal(),numberOfSignal()) ;
  D.setIdentity() ;
  vpMatrix DA, DAmemory ;
  vpColVector DAx ;
  vpColVector w(numberOfSignal()) ;
  w =1 ;
  int nos_1 = numberOfSignal() ;

  if (list.nbElement() < 3)
  {
    vpERROR_TRACE("Not enough point") ;
    throw(vpTrackingException(vpTrackingException::notEnoughPointError,
			      "not enough point")) ;
  }

  if (circle ==false)
  {
    vpMatrix A(numberOfSignal(),5) ;
    vpColVector x(5);

    list.front() ;
    int k =0 ;
    for (i=0 ; i < list.nbElement() ; i++)
    {
      p = list.value() ;
      if (p.suppress==0)
      {

        A[k][0] = vpMath::sqr(p.jfloat) ;
        A[k][1] = 2 * p.ifloat * p.jfloat ;
        A[k][2] = 2 * p.ifloat ;
        A[k][3] = 2 * p.jfloat ;
        A[k][4] = 1 ;

        b[k] = - vpMath::sqr(p.ifloat) ;
        k++ ;
      }
      list.next() ;
    }

    while (iter < 4 )
    {
      DA = D*A ;
      vpMatrix DAp ;

      x = DA.pseudoInverse(1e-26) *D*b ;

      vpColVector residu(nos_1);
      residu = b - A*x;
      r.setIteration(iter) ;
      r.MEstimator(vpRobust::TUKEY,residu,w) ;

      k = 0;
      for (i=0 ; i < nos_1 ; i++)
      {
        D[k][k] =w[k]  ;
        k++;
      }
      iter++;
    }

    list.front() ;
    k =0 ;
    for (i=0 ; i < list.nbElement() ; i++)
    {
      p = list.value() ;
      if (p.suppress==0)
      {
        if (w[k] < 0.2)
        {
          p.suppress  = 3 ;
          list.modify(p) ;
        }
        k++ ;
      }
      list.next() ;
    }
    for(i = 0; i < 5; i ++)
      K[i] = x[i];
  }
  
  else
  {
    vpMatrix A(numberOfSignal(),3) ;
    vpColVector x(3);

    list.front() ;
    int k =0 ;
    for (i=0 ; i < list.nbElement() ; i++)
    {
      p = list.value() ;
      if (p.suppress==0)
      {

        A[k][0] = 2* p.ifloat ;
        A[k][1] = 2 * p.jfloat ;
        A[k][2] = 1 ;

        b[k] = - vpMath::sqr(p.ifloat) - vpMath::sqr(p.jfloat) ;
        k++ ;
      }
      list.next() ;
    }

    while (iter < 4 )
    {
      DA = D*A ;
      vpMatrix DAp ;

      x = DA.pseudoInverse(1e-26) *D*b ;

      vpColVector residu(nos_1);
      residu = b - A*x;
      r.setIteration(iter) ;
      r.MEstimator(vpRobust::TUKEY,residu,w) ;

      k = 0;
      for (i=0 ; i < nos_1 ; i++)
      {
        D[k][k] =w[k];
        k++;
      }
      iter++;
    }

    list.front() ;
    k =0 ;
    for (i=0 ; i < list.nbElement() ; i++)
    {
      p = list.value() ;
      if (p.suppress==0)
      {
        if (w[k] < 0.2)
        {
          p.suppress  = 3 ;
          list.modify(p) ;
        }
        k++ ;
      }
      list.next() ;
    }
    for(i = 0; i < 3; i ++)
      K[i+2] = x[i];
  }
  getParameters() ;
}


/*!
  Display the ellipse.

  \warning To effectively display the ellipse a call to
  vpDisplay::flush() is needed.

  \param I : Image in which the ellipse appears.
  \param col : Color of the displayed ellipse.
 */
void
vpMeEllipse::display(vpImage<unsigned char> &I, vpColor col)
{

  double j1, i1;
  vpImagePoint iP11;
  double j2, i2;
  vpImagePoint iP22;
  j1 = j2 = i1 = i2 = 0 ;

  double incr = vpMath::rad(2) ; // angle increment

  vpDisplay::displayCross(I,iPc,20,vpColor::red) ;


  double k = alpha1 ;
  while (k+incr<alpha2)
  {
    j1 = a *cos(k) ; // equation of an ellipse
    i1 = b *sin(k) ; // equation of an ellipse

    j2 = a *cos(k+incr) ; // equation of an ellipse
    i2 = b *sin(k+incr) ; // equation of an ellipse

    // (i1,j1) are the coordinates on the origin centered ellipse ;
    // a rotation by "e" and a translation by (xci,jc) are done
    // to get the coordinates of the point on the shifted ellipse
    iP11.set_j ( iPc.get_j() + ce *j1 - se *i1 );
    iP11.set_i ( iPc.get_i() -( se *j1 + ce *i1) );
    // to get the coordinates of the point on the shifted ellipse
    iP22.set_j ( iPc.get_j() + ce *j2 - se *i2 );
    iP22.set_i ( iPc.get_i() -( se *j2 + ce *i2) );


    vpDisplay::displayLine(I, iP11, iP22, col, 3) ;

    k += incr ;
  }

    j1 = a *cos(alpha1) ; // equation of an ellipse
    i1 = b *sin(alpha1) ; // equation of an ellipse

    j2 = a *cos(alpha2) ; // equation of an ellipse
    i2 = b *sin(alpha2) ; // equation of an ellipse

    // (i1,j1) are the coordinates on the origin centered ellipse ;
    // a rotation by "e" and a translation by (xci,jc) are done
    // to get the coordinates of the point on the shifted ellipse
    iP11.set_j ( iPc.get_j() + ce *j1 - se *i1 );
    iP11.set_i ( iPc.get_i() -( se *j1 + ce *i1) );
    // to get the coordinates of the point on the shifted ellipse
    iP22.set_j ( iPc.get_j() + ce *j2 - se *i2 );
    iP22.set_i ( iPc.get_i() -( se *j2 + ce *i2) );


    vpDisplay::displayLine(I,iPc, iP11, vpColor::red, 3) ;
    vpDisplay::displayLine(I,iPc, iP22, vpColor::blue, 3) ;
}


/*!
  Initilization of the tracking. Ask the user to click on five points
  from the ellipse edge to track.

  \param I : Image in which the ellipse appears.
*/
void
vpMeEllipse::initTracking(vpImage<unsigned char> &I)
{
  vpCDEBUG(1) <<" begin vpMeEllipse::initTracking()"<<std::endl ;

  int n=5 ;
  vpImagePoint *iP;
  iP = new vpImagePoint[n];

  for (int k =0 ; k < n ; k++)
    {
      std::cout << "Click points "<< k+1 <<"/" << n ;
      std::cout << " on the ellipse in the trigonometric order" <<std::endl ;
      while (vpDisplay::getClick(I,iP[k])!=true) ;
      std::cout << iP[k] << std::endl;
    }

  iP1 = iP[0];
  iP2 = iP[n-1];

  initTracking(I, n, iP) ;

  delete [] iP;
}


/*!
  Initialization of the tracking. The ellipse is defined thanks to the
  coordinates of n points.

  \warning It is better to use at least five points to well estimate the K parameters.

  \param I : Image in which the ellipse appears.
  \param n : The number of points in the list.
  \param iP : A pointer to a list of pointsbelonging to the ellipse edge.
*/
void
vpMeEllipse::initTracking(vpImage<unsigned char> &I, int n,
			  vpImagePoint *iP)
{
  vpCDEBUG(1) <<" begin vpMeEllipse::initTracking()"<<std::endl ;

  if (circle==false)
  {
    vpMatrix A(n,5) ;
    vpColVector b(n) ;
    vpColVector x(5) ;

    // Construction du systeme Ax=b
    //! i^2 + K0 j^2 + 2 K1 i j + 2 K2 i + 2 K3 j + K4
    // A = (j^2 2ij 2i 2j 1)   x = (K0 K1 K2 K3 K4)^T  b = (-i^2 )

    for (int k =0 ; k < n ; k++)
    {
      A[k][0] = vpMath::sqr(iP[k].get_j()) ;
      A[k][1] = 2* iP[k].get_i() * iP[k].get_j() ;
      A[k][2] = 2* iP[k].get_i() ;
      A[k][3] = 2* iP[k].get_j() ;
      A[k][4] = 1 ;

      b[k] = - vpMath::sqr(iP[k].get_i()) ;
    }

    K = A.pseudoInverse(1e-26)*b ;
    std::cout << K << std::endl;
  }
  else
  {
    vpMatrix A(n,3) ;
    vpColVector b(n) ;
    vpColVector x(3) ;

    vpColVector Kc(3) ;
    for (int k =0 ; k < n ; k++)
    {
      A[k][0] =  2* iP[k].get_i() ;
      A[k][1] =  2* iP[k].get_j() ;
      A[k][2] = 1 ;

      b[k] = - vpMath::sqr(iP[k].get_i()) - vpMath::sqr(iP[k].get_j()) ;
    }

    Kc = A.pseudoInverse(1e-26)*b ;
    K[0] = 1 ;
    K[1] = 0 ;
    K[2] = Kc[0] ;
    K[3] = Kc[1] ;
    K[4] = Kc[2] ;

    std::cout << K << std::endl;
  }

  iP1 = iP[0];
  iP2 = iP[n-1];

  getParameters() ;
  computeAngle(iP1, iP2) ;
  display(I, vpColor::green) ;

  sample(I) ;

  //  2. On appelle ce qui n'est pas specifique
  {
    vpMeTracker::initTracking(I) ;
  }


  try{
    track(I) ;
  }
  catch(...)
  {
    vpERROR_TRACE("Error caught") ;
    throw ;
  }
  vpMeTracker::display(I) ;
  vpDisplay::flush(I) ;

}

/*!
  Track the ellipse in the image I.

  \param I : Image in which the ellipse appears.
*/
void
vpMeEllipse::track(vpImage<unsigned char> &I)
{
  vpCDEBUG(1) <<"begin vpMeEllipse::track()"<<std::endl ;

  static int iter =0 ;
  //  1. On fait ce qui concerne les ellipse (peut etre vide)
  {
  }

  vpDisplay::display(I) ;
  //  2. On appelle ce qui n'est pas specifique
  {

  try{
       vpMeTracker::track(I) ;
  }
  catch(...)
  {
    vpERROR_TRACE("Error caught") ;
    throw ;
  }
    //    std::cout << "number of signals " << numberOfSignal() << std::endl ;
  }

  // 3. On revient aux ellipses
  {
    // Estimation des parametres de la droite aux moindres carre
    suppressPoints() ;
    setExtremities() ;


    try{
      leastSquare() ;  }
    catch(...)
    {
      vpERROR_TRACE("Error caught") ;
      throw ;
    }

    seekExtremities(I) ;

    setExtremities() ;

    try
    {
      leastSquare() ;
    }
    catch(...)
    {
      vpERROR_TRACE("Error caught") ;
	  throw ;
    }

    // suppression des points rejetes par la regression robuste
    suppressPoints() ;
    setExtremities() ;

    //reechantillonage si necessaire
    reSample(I) ;

    // remet a jour l'angle delta pour chaque  point de la liste

    updateTheta() ;
    
    computeMoments();

    // Remise a jour de delta dans la liste de site me
    if (vpDEBUG_ENABLE(2))
    {
	display(I,vpColor::red) ;
	vpMeTracker::display(I) ;
	vpDisplay::flush(I) ;
    }
//     computeAngle(iP1, iP2) ;
// 
//     if (iter%5==0)
//     {
//       sample(I) ;
//       try{
// 	leastSquare() ;  }
//       catch(...)
//       {
// 	vpERROR_TRACE("Error caught") ;
// 	throw ;
//       }
//       computeAngle(iP1, iP2) ;
//     }
//     seekExtremities(I) ;
// 
//     vpMeTracker::display(I) ;
//     // vpDisplay::flush(I) ;
// 
//     // remet a jour l'angle theta pour chaque  point de la liste
//     updateTheta() ;

  }

  iter++ ;


  vpCDEBUG(1) << "end vpMeEllipse::track()"<<std::endl ;

}


/*!
  Computes the 0 order moment \f$ m_{00} \f$ which represents the area of the ellipse.
  
  Computes the second central moments \f$ \mu_{20} \f$, \f$ \mu_{02} \f$ and \f$ \mu_{11} \f$
*/
void
vpMeEllipse::computeMoments()
{
  double tane = tan(-1/e);
  m00 = M_PI*a*b;
  m10 = m00*iPc.get_i();
  m01 = m00*iPc.get_j();
  m20 = m00*(a*a+b*b*tane*tane)/(4*(1+tane*tane))+m00*iPc.get_i()*iPc.get_i();
  m02 = m00*(a*a*tane*tane+b*b)/(4*(1+tane*tane))+m00*iPc.get_j()*iPc.get_j();
  m11 = m00*tane*(a*a-b*b)/(4*(1+tane*tane))+m00*iPc.get_i()*iPc.get_j();
  mu11 = m11 - iPc.get_j()*m10;
  mu02 = m02 - iPc.get_j()*m01;
  mu20 = m20 - iPc.get_i()*m10;
}








#ifdef VISP_BUILD_DEPRECATED_FUNCTIONS

/*!
 * \brief computeAngle
 */
void
vpMeEllipse::computeAngle(int ip1, int jp1, double &_alpha1,
			  int ip2, int jp2, double &_alpha2)
{

  getParameters() ;
  double j1, i1, j11, i11;
  j1 =  i1 =  0.0 ;

  int number_of_points = 2000 ;
  double incr = 2 * M_PI / number_of_points ; // angle increment

  double dmin1 = 1e6  ;
  double dmin2 = 1e6  ;

  double k =  -M_PI ;
  while(k < M_PI) {

    j1 = a *cos(k) ; // equation of an ellipse
    i1 = b *sin(k) ; // equation of an ellipse

    // (i1,j1) are the coordinates on the origin centered ellipse ;
    // a rotation by "e" and a translation by (xci,jc) are done
    // to get the coordinates of the point on the shifted ellipse
    j11 = iPc.get_j() + ce *j1 - se *i1 ;
    i11 = iPc.get_i() -( se *j1 + ce *i1) ;

    double  d = vpMath::sqr(ip1-i11) + vpMath::sqr(jp1-j11) ;
    if (d < dmin1)
    {
      dmin1 = d ;
      alpha1 = k ;
      _alpha1 = k ;
    }
    d = vpMath::sqr(ip2-i11) + vpMath::sqr(jp2-j11) ;
    if (d < dmin2)
    {
      dmin2 = d ;
      alpha2 = k ;
      _alpha2 = k ;
    }
    k += incr ;
  }

  if (alpha2 <alpha1) alpha2 += 2*M_PI ;

  vpCDEBUG(1) << "end vpMeEllipse::computeAngle(..)" << alpha1 << "  " << alpha2 << std::endl ;

}


/*!
 * \brief computeAngle
 */
void
vpMeEllipse::computeAngle(int ip1, int jp1, int ip2, int jp2)
{

  double a1, a2 ;
  computeAngle(ip1,jp1,a1, ip2, jp2,a2) ;
}


void
vpMeEllipse::initTracking(vpImage<unsigned char> &I, int n,
			  unsigned *i, unsigned *j)
{
  vpCDEBUG(1) <<" begin vpMeEllipse::initTracking()"<<std::endl ;

  if (circle==false)
  {
    vpMatrix A(n,5) ;
    vpColVector b(n) ;
    vpColVector x(5) ;

    // Construction du systeme Ax=b
    //! i^2 + K0 j^2 + 2 K1 i j + 2 K2 i + 2 K3 j + K4
    // A = (j^2 2ij 2i 2j 1)   x = (K0 K1 K2 K3 K4)^T  b = (-i^2 )

    for (int k =0 ; k < n ; k++)
    {
      A[k][0] = vpMath::sqr(j[k]) ;
      A[k][1] = 2* i[k] * j[k] ;
      A[k][2] = 2* i[k] ;
      A[k][3] = 2* j[k] ;
      A[k][4] = 1 ;

      b[k] = - vpMath::sqr(i[k]) ;
    }

    K = A.pseudoInverse(1e-26)*b ;
    std::cout << K << std::endl;
  }
  else
  {
    vpMatrix A(n,3) ;
    vpColVector b(n) ;
    vpColVector x(3) ;

    vpColVector Kc(3) ;
    for (int k =0 ; k < n ; k++)
    {
      A[k][0] =  2* i[k] ;
      A[k][1] =  2* j[k] ;

      A[k][2] = 1 ;
      b[k] = - vpMath::sqr(i[k]) - vpMath::sqr(j[k]) ;
    }

    Kc = A.pseudoInverse(1e-26)*b ;
    K[0] = 1 ;
    K[1] = 0 ;
    K[2] = Kc[0] ;
    K[3] = Kc[1] ;
    K[4] = Kc[2] ;

    std::cout << K << std::endl;
  }
  iP1.set_i( i[0] );
  iP1.set_j( j[0] );
  iP2.set_i( i[n-1] );
  iP2.set_j( j[n-1] );

  getParameters() ;
  computeAngle(iP1, iP2) ;
  display(I, vpColor::green) ;

  sample(I) ;

  //  2. On appelle ce qui n'est pas specifique
  {
    vpMeTracker::initTracking(I) ;
  }


  try{
    track(I) ;
  }
  catch(...)
  {
    vpERROR_TRACE("Error caught") ;
    throw ;
  }
  vpMeTracker::display(I) ;
  vpDisplay::flush(I) ;

}

#endif

