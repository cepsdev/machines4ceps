/* out_frames.hpp 
   CREATED Tue Mar 14 16:11:50 2017

   GENERATED BY THE sm4ceps C++ GENERATOR VERSION 0.90.
   BASED ON cepS (c) 2017 Tomas Prerovsky <tomas.prerovsky@gmail.com> VERSION 1.1 (Mar 10 2017) BUILT WITH GCC 5.2.1 20151010 on GNU/LINUX 64BIT (C) BY THE AUTHORS OF ceps (ceps is hosted at github: https://github.com/cepsdev/ceps.git) 

   Input files:
      prelude.ceps
      can3.xml

   THIS IS A GENERATED FILE.

   *** DO NOT MODIFY. ***
*/


#ifndef INC_SM4CEPS_GENERATED_OUT_FRAMES_HPP 
#define INC_SM4CEPS_GENERATED_OUT_FRAMES_HPP
namespace raw_frm_dcls{
 struct Engine_RPM_in{
  unsigned short pos_1:16;
  unsigned char pos_2:8;
  unsigned char pos_3:8;
 }__attribute__ ((__packed__));
 struct Engine_RPM_out{
  unsigned short pos_1:16;
  unsigned char pos_2:8;
  unsigned char pos_3:8;
 }__attribute__ ((__packed__));

}
#endif