/***********************************************************************
 * $Id: det_monomial.c 1764 2011-04-21 14:16:16Z deuzeman $
 *
 * Copyright (C) 2011 Elena Garcia-Ramos
 *
 * This file is part of tmLQCD.
 *
 * tmLQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * tmLQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with tmLQCD.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#ifndef _SPECTRAL_PROJECTOR_H
#define _SPECTRAL_PROJECTOR_H

#include "su3.h"

extern double mode_n;

double mode_number(spinor * const S, double const mstarsq);

void top_sus(spinor * const S, double const mstarsq);

#endif
