// Copyright 2016 Mark Owen
// http://www.quinapalus.com
// E-mail: qfp@quinapalus.com
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License
// as published by the Free Software Foundation.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file.  If not, see <http://www.gnu.org/licenses/> or
// write to the Free Software Foundation, Inc., 51 Franklin Street,
// Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _QFPLIB_M3_H_
#define _QFPLIB_M3_H_

#ifdef __cplusplus
  extern "C" {
#endif

extern float qfp_fadd(float x, float y);    // 单精度加法：x + y
extern float qfp_fsub(float x, float y);    // 单精度减法：x - y
extern float qfp_fmul(float x, float y);    // 单精度乘法：x * y
extern float qfp_fdiv(float x, float y);    // 单精度除法：x / y
extern float qfp_fsqrt(float x);            // 单精度平方根：√x
extern float qfp_fexp(float x);             // 单精度指数：e^x
extern float qfp_fln(float x);              // 单精度自然对数：ln(x) (x>0)
extern float qfp_fsin(float x);             // 单精度正弦：sin(x) (x 弧度)
extern float qfp_fcos(float x);             // 单精度余弦：cos(x) (x 弧度)
extern float qfp_ftan(float x);             // 单精度正切：tan(x) (x 弧度)
extern float qfp_fatan2(float y, float x);  // 单精度反正切：atan2(y, x)，返回 (-π, π] 弧度

#ifdef __cplusplus
  } // extern "C"
#endif
#endif
