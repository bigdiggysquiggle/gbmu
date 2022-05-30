/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_debug.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alan <alanbarnett328@gmail.com>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 06:52:56 by alan              #+#    #+#             */
/*   Updated: 2020/10/23 12:48:09 by alan             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PRINT_DEBUG_H
# define PRINT_DEBUG_H

# ifdef DEBUG

#  include <cstdio>
#  include <unistd.h>

#  define FORMAT(fmt) "debug(%s:%s): "#fmt"\n", __FILE__, __func__
#  define PRINT_DEBUG(fmt, args...) dprintf(STDERR_FILENO, FORMAT(fmt), ##args)

# else

/*
** This is set to void 0, because the compiler will complain of an expected
** expression before a semicolon if there isn't anything. I also have to leave
** the semicolon on the line, because norm will complain that the file may not
** compile for some reasons if there are any lines without semicolons.
*/

#  define PRINT_DEBUG(fmt, args...) (void)0

# endif

#endif
