/*
** v_font.cpp
** Font management
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** Copyright 2005-2019 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include <cwctype>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "templates.h"
#include "m_swap.h"
#include "v_font.h"
#include "cmdlib.h"
#include "sc_man.h"
#include "v_text.h"
#include "image.h"
#include "utf8.h"
#include "myiswalpha.h"
#include "fontchars.h"
#include "imagehelpers.h"
#include "glbackend/glbackend.h"

#include "fontinternals.h"



//==========================================================================
//
// FFont :: ~FFont
//
//==========================================================================

FFont::~FFont ()
{
	FFont **prev = &FirstFont;
	FFont *font = *prev;

	while (font != nullptr && font != this)
	{
		prev = &font->Next;
		font = *prev;
	}

	if (font != nullptr)
	{
		*prev = font->Next;
	}
}

//==========================================================================
//
// FFont :: CheckCase
//
//==========================================================================

void FFont::CheckCase()
{
	int lowercount = 0, uppercount = 0;
	for (unsigned i = 0; i < Chars.Size(); i++)
	{
		unsigned chr = i + FirstChar;
		if (lowerforupper[chr] == chr && upperforlower[chr] == chr)
		{
			continue;	// not a letter;
		}
		if (myislower(chr))
		{
			if (Chars[i].TranslatedPic != nullptr) lowercount++;
		}
		else
		{
			if (Chars[i].TranslatedPic != nullptr) uppercount++;
		}
	}
	if (lowercount == 0) return;	// This is an uppercase-only font and we are done.

	// The ß needs special treatment because it is far more likely to be supplied lowercase only, even in an uppercase font.
	if (Chars[0xdf - FirstChar].TranslatedPic != nullptr)
	{
		if (LastChar < 0x1e9e)
		{
			Chars.Resize(0x1e9f - FirstChar);
			LastChar = 0x1e9e;
		}
		if (Chars[0x1e9e - FirstChar].TranslatedPic == nullptr)
		{
			std::swap(Chars[0xdf - FirstChar], Chars[0x1e9e - FirstChar]);
			lowercount--;
			uppercount++;
			if (lowercount == 0) return;
		}
	}
}

//==========================================================================
//
// FFont :: FindFont
//
// Searches for the named font in the list of loaded fonts, returning the
// font if it was found. The disk is not checked if it cannot be found.
//
//==========================================================================

FFont *FFont::FindFont (FName name)
{
	if (name == NAME_None)
	{
		return nullptr;
	}
	FFont *font = FirstFont;

	while (font != nullptr)
	{
		if (font->FontName == name) return font;
		font = font->Next;
	}
	return nullptr;
}

//==========================================================================
//
// RecordTextureColors
//
// Given a 256 entry buffer, sets every entry that corresponds to a color
// used by the texture to 1.
//
//==========================================================================

void RecordTextureColors (FTexture *pic, uint32_t *usedcolors)
{
	auto size = pic->GetWidth() * pic->GetHeight();
	TArray<uint8_t> pixels(size, 1);
	int x;
	
	pic->Create8BitPixels(pixels.Data());
	
	for(x = 0;x < size; x++)
	{
		usedcolors[pixels[x]]++;
	}
}

//==========================================================================
//
// RecordAllTextureColors
//
// Given a 256 entry buffer, sets every entry that corresponds to a color
// used by the font.
//
//==========================================================================

void FFont::RecordAllTextureColors(uint32_t *usedcolors)
{
	for (unsigned int i = 0; i < Chars.Size(); i++)
	{
		if (Chars[i].TranslatedPic)
		{
			FFontChar1 *pic = static_cast<FFontChar1 *>(Chars[i].TranslatedPic);
			if (pic)
			{
				// The remap must be temporarily reset here because this can be called on an initialized font.
				auto sr = pic->ResetSourceRemap();
				RecordTextureColors(pic, usedcolors);
				pic->SetSourceRemap(sr);
			}
		}
	}
}

//==========================================================================
//
// SetDefaultTranslation
//
// Builds a translation to map the stock font to a mod provided replacement.
//
//==========================================================================

void FFont::SetDefaultTranslation(uint32_t *othercolors)
{
	uint32_t mycolors[256] = {};
	RecordAllTextureColors(mycolors);

	uint8_t mytranslation[256], othertranslation[256], myreverse[256], otherreverse[256];
	TArray<double> myluminosity, otherluminosity;

	SimpleTranslation(mycolors, mytranslation, myreverse, myluminosity);
	SimpleTranslation(othercolors, othertranslation, otherreverse, otherluminosity);

	FRemapTable remap;
	remap.Palette[0] = 0;

	for (unsigned l = 1; l < myluminosity.Size(); l++)
	{
		for (unsigned o = 1; o < otherluminosity.Size()-1; o++)	// luminosity[0] is for the transparent color
		{
			if (myluminosity[l] >= otherluminosity[o] && myluminosity[l] <= otherluminosity[o+1])
			{
				PalEntry color1 = ImageHelpers::BasePalette[otherreverse[o]];
				PalEntry color2 = ImageHelpers::BasePalette[otherreverse[o+1]];
				double weight = 0;
				if (otherluminosity[o] != otherluminosity[o + 1])
				{
					weight = (myluminosity[l] - otherluminosity[o]) / (otherluminosity[o + 1] - otherluminosity[o]);
				}
				int r = int(color1.r + weight * (color2.r - color1.r));
				int g = int(color1.g + weight * (color2.g - color1.g));
				int b = int(color1.b + weight * (color2.b - color1.b));

				r = clamp(r, 0, 255);
				g = clamp(g, 0, 255);
				b = clamp(b, 0, 255);
				remap.Palette[l] = PalEntry(255, r, g, b);
				break;
			}
		}
	}
	Ranges[CR_UNTRANSLATED] = GLInterface.GetPaletteIndex(remap.Palette);
	forceremap = true;
}


//==========================================================================
//
// compare
//
// Used for sorting colors by brightness.
//
//==========================================================================

static int compare (const void *arg1, const void *arg2)
{
	if (RPART(ImageHelpers::BasePalette[*((uint8_t *)arg1)]) * 299 +
		GPART(ImageHelpers::BasePalette[*((uint8_t *)arg1)]) * 587 +
		BPART(ImageHelpers::BasePalette[*((uint8_t *)arg1)]) * 114  <
		RPART(ImageHelpers::BasePalette[*((uint8_t *)arg2)]) * 299 +
		GPART(ImageHelpers::BasePalette[*((uint8_t *)arg2)]) * 587 +
		BPART(ImageHelpers::BasePalette[*((uint8_t *)arg2)]) * 114)
		return -1;
	else
		return 1;
}

//==========================================================================
//
// FFont :: SimpleTranslation
//
// Colorsused, translation, and reverse must all be 256 entry buffers.
// Colorsused must already be filled out.
// Translation be set to remap the source colors to a new range of
// consecutive colors based at 1 (0 is transparent).
// Reverse will be just the opposite of translation: It maps the new color
// range to the original colors.
// *Luminosity will be an array just large enough to hold the brightness
// levels of all the used colors, in consecutive order. It is sorted from
// darkest to lightest and scaled such that the darkest color is 0.0 and
// the brightest color is 1.0.
// The return value is the number of used colors and thus the number of
// entries in *luminosity.
//
//==========================================================================

int FFont::SimpleTranslation (uint32_t *colorsused, uint8_t *translation, uint8_t *reverse, TArray<double> &Luminosity)
{
	double min, max, diver;
	int i, j;

	memset (translation, 0, 256);

	reverse[0] = 0;
	for (i = 1, j = 1; i < 256; i++)
	{
		if (colorsused[i])
		{
			reverse[j++] = i;
		}
	}

	qsort (reverse+1, j-1, 1, compare);

	Luminosity.Resize(j);
	Luminosity[0] = 0.0; // [BL] Prevent uninitalized memory
	max = 0.0;
	min = 100000000.0;
	for (i = 1; i < j; i++)
	{
		translation[reverse[i]] = i;

		Luminosity[i] = RPART(ImageHelpers::BasePalette[reverse[i]]) * 0.299 +
						   GPART(ImageHelpers::BasePalette[reverse[i]]) * 0.587 +
						   BPART(ImageHelpers::BasePalette[reverse[i]]) * 0.114;
		if (Luminosity[i] > max)
			max = Luminosity[i];
		if (Luminosity[i] < min)
			min = Luminosity[i];
	}
	diver = 1.0 / (max - min);
	for (i = 1; i < j; i++)
	{
		Luminosity[i] = (Luminosity[i] - min) * diver;
	}

	return j;
}

//==========================================================================
//
// FFont :: BuildTranslations
//
// Build color translations for this font. Luminosity is an array of
// brightness levels. The ActiveColors member must be set to indicate how
// large this array is. Identity is an array that remaps the colors to
// their original values; it is only used for CR_UNTRANSLATED. Ranges
// is an array of TranslationParm structs defining the ranges for every
// possible color, in order. Palette is the colors to use for the
// untranslated version of the font.
//
//==========================================================================

void FFont::BuildTranslations (const double *luminosity, const uint8_t *identity,
							   const void *ranges, int total_colors, const PalEntry *palette)
{
	int i, j;
	const TranslationParm *parmstart = (const TranslationParm *)ranges;

	FRemapTable remap;

	// Create different translations for different color ranges
	Ranges.Clear();
	for (i = 0; i < NumTextColors; i++)
	{
		if (i == CR_UNTRANSLATED)
		{
			if (identity != nullptr)
			{
				if (palette != nullptr)
				{
					memcpy (remap.Palette, palette, ActiveColors*sizeof(PalEntry));
				}
				else
				{
					remap.Palette[0] = ImageHelpers::BasePalette[identity[0]] & MAKEARGB(0,255,255,255);
					for (j = 1; j < ActiveColors; ++j)
					{
						remap.Palette[j] = ImageHelpers::BasePalette[identity[j]] | MAKEARGB(255,0,0,0);
					}
				}
			}
			else
			{
			}
			Ranges.Push(GLInterface.GetPaletteIndex(remap.Palette));
			continue;
		}

		assert(parmstart->RangeStart >= 0);

		remap.Palette[0] = 0;

		for (j = 1; j < ActiveColors; j++)
		{
			int v = int(luminosity[j] * 256.0);

			// Find the color range that this luminosity value lies within.
			const TranslationParm *parms = parmstart - 1;
			do
			{
				parms++;
				if (parms->RangeStart <= v && parms->RangeEnd >= v)
					break;
			}
			while (parms[1].RangeStart > parms[0].RangeEnd);

			// Linearly interpolate to find out which color this luminosity level gets.
			int rangev = ((v - parms->RangeStart) << 8) / (parms->RangeEnd - parms->RangeStart);
			int r = ((parms->Start[0] << 8) + rangev * (parms->End[0] - parms->Start[0])) >> 8; // red
			int g = ((parms->Start[1] << 8) + rangev * (parms->End[1] - parms->Start[1])) >> 8; // green
			int b = ((parms->Start[2] << 8) + rangev * (parms->End[2] - parms->Start[2])) >> 8; // blue
			r = clamp(r, 0, 255);
			g = clamp(g, 0, 255);
			b = clamp(b, 0, 255);
			remap.Palette[j] = PalEntry(255,r,g,b);
		}

		Ranges.Push(GLInterface.GetPaletteIndex(remap.Palette));

		// Advance to the next color range.
		while (parmstart[1].RangeStart > parmstart[0].RangeEnd)
		{
			parmstart++;
		}
		parmstart++;
	}
}

//==========================================================================
//
// FFont :: GetColorTranslation
//
//==========================================================================

int FFont::GetColorTranslation (EColorRange range, PalEntry *color) const
{
	if (noTranslate)
	{
		PalEntry retcolor = PalEntry(255, 255, 255, 255);
		if (range >= 0 && range < NumTextColors && range != CR_UNTRANSLATED)
		{
			retcolor = TranslationColors[range];
			retcolor.a = 255;
		}
		if (color != nullptr) *color = retcolor;
	}
	if (ActiveColors == 0)
		return -1;
	else if (range >= NumTextColors)
		range = CR_UNTRANSLATED;
	//if (range == CR_UNTRANSLATED && !translateUntranslated) return nullptr;
	return Ranges[range];
}

//==========================================================================
//
// FFont :: GetCharCode
//
// If the character code is in the font, returns it. If it is not, but it
// is lowercase and has an uppercase variant present, return that. Otherwise
// return -1.
//
//==========================================================================

int FFont::GetCharCode(int code, bool needpic) const
{
	if (code < 0 && code >= -128)
	{
		// regular chars turn negative when the 8th bit is set.
		code &= 255;
	}
	if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].TranslatedPic != nullptr))
	{
		return code;
	}
	
	// Use different substitution logic based on the fonts content:
	// In a font which has both upper and lower case, prefer unaccented small characters over capital ones.
	// In a pure upper-case font, do not check for lower case replacements.
	if (!MixedCase)
	{
		// Try converting lowercase characters to uppercase.
		if (myislower(code))
		{
			code = upperforlower[code];
			if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].TranslatedPic != nullptr))
			{
				return code;
			}
		}
		// Try stripping accents from accented characters.
		int newcode = stripaccent(code);
		if (newcode != code)
		{
			code = newcode;
			if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].TranslatedPic != nullptr))
			{
				return code;
			}
		}
	}
	else
	{
		int originalcode = code;
		int newcode;

		// Try stripping accents from accented characters. This may repeat to allow multi-step fallbacks.
		while ((newcode = stripaccent(code)) != code)
		{
			code = newcode;
			if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].TranslatedPic != nullptr))
			{
				return code;
			}
		}

		code = originalcode;
		if (myislower(code))
		{
			int upper = upperforlower[code];
			// Stripping accents did not help - now try uppercase for lowercase
			if (upper != code) return GetCharCode(upper, needpic);
		}

		// Same for the uppercase character. Since we restart at the accented version this must go through the entire thing again.
		while ((newcode = stripaccent(code)) != code)
		{
			code = newcode;
			if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].TranslatedPic != nullptr))
			{
				return code;
			}
		}

	}

	return -1;
}

//==========================================================================
//
// FFont :: GetChar
//
//==========================================================================

FTexture *FFont::GetChar (int code, int translation, int *const width, bool *redirected) const
{
	code = GetCharCode(code, true);
	int xmove = SpaceWidth;

	if (code >= 0)
	{
		code -= FirstChar;
		xmove = Chars[code].XMove;
	}
	
	if (width != nullptr)
	{
		*width = xmove;
	}
	if (code < 0) return nullptr;


	if (translation == CR_UNTRANSLATED && !forceremap)
	{
		bool redirect = Chars[code].OriginalPic && Chars[code].OriginalPic != Chars[code].TranslatedPic;
		if (redirected) *redirected = redirect;
		if (redirect)
		{
			return Chars[code].OriginalPic;
		}
	}
	if (redirected) *redirected = false;
	return Chars[code].TranslatedPic;
}

//==========================================================================
//
// FFont :: CharWidth
//
//==========================================================================

int FFont::CharWidth (int code) const
{
	code = GetCharCode(code, true);
	if (code >= 0) return Chars[code - FirstChar].XMove;
	return SpaceWidth;
}

//==========================================================================
//
// 
//
//==========================================================================

double GetBottomAlignOffset(FFont *font, int c)
{
	int w;
	FTexture *tex_zero = font->GetChar('0', CR_UNDEFINED, &w);
	FTexture *texc = font->GetChar(c, CR_UNDEFINED, &w);
	double offset = 0;
	if (texc) offset += texc->GetTopOffset();
	if (tex_zero) offset += -tex_zero->GetTopOffset() + tex_zero->GetHeight();
	return offset;
}

//==========================================================================
//
// Checks if the font contains proper glyphs for all characters in the string
//
//==========================================================================

bool FFont::CanPrint(const uint8_t *string) const
{
	if (!string) return true;
	while (*string)
	{
		auto chr = GetCharFromString(string);
		if (!MixedCase) chr = upperforlower[chr];	// For uppercase-only fonts we shouldn't check lowercase characters.
		if (chr == TEXTCOLOR_ESCAPE)
		{
			// We do not need to check for UTF-8 in here.
			if (*string == '[')
			{
				while (*string != '\0' && *string != ']')
				{
					++string;
				}
			}
			if (*string != '\0')
			{
				++string;
			}
			continue;
		}
		else if (chr != '\n')
		{
			int cc = GetCharCode(chr, true);
			if (chr != cc && myiswalpha(chr))// && cc != getAlternative(chr))
			{
				return false;
			}
		}
	}

	return true;
}

//==========================================================================
//
// Find string width using this font
//
//==========================================================================

int FFont::StringWidth(const uint8_t *string) const
{
	int w = 0;
	int maxw = 0;

	while (*string)
	{
		auto chr = GetCharFromString(string);
		if (chr == TEXTCOLOR_ESCAPE)
		{
			// We do not need to check for UTF-8 in here.
			if (*string == '[')
			{
				while (*string != '\0' && *string != ']')
				{
					++string;
				}
			}
			if (*string != '\0')
			{
				++string;
			}
			continue;
		}
		else if (chr == '\n')
		{
			if (w > maxw)
				maxw = w;
			w = 0;
		}
		else
		{
			w += CharWidth(chr) + GlobalKerning;
		}
	}

	return std::max(maxw, w);
}

//==========================================================================
//
// Get the largest ascender in the first line of this text.
//
//==========================================================================

int FFont::GetMaxAscender(const uint8_t* string) const
{
	int retval = 0;

	while (*string)
	{
		auto chr = GetCharFromString(string);
		if (chr == TEXTCOLOR_ESCAPE)
		{
			// We do not need to check for UTF-8 in here.
			if (*string == '[')
			{
				while (*string != '\0' && *string != ']')
				{
					++string;
				}
			}
			if (*string != '\0')
			{
				++string;
			}
			continue;
		}
		else if (chr == '\n')
		{
			break;
		}
		else
		{
			auto ctex = GetChar(chr, CR_UNTRANSLATED, nullptr);
			if (ctex)
			{
				auto offs = int(ctex->GetTopOffset());
				if (offs > retval) retval = offs;
			}
		}
	}

	return retval;
}

//==========================================================================
//
// FFont :: LoadTranslations
//
//==========================================================================

void FFont::LoadTranslations()
{
	unsigned int count = LastChar - FirstChar + 1;
	uint32_t usedcolors[256] = {};
	uint8_t identity[256];
	TArray<double> Luminosity;

	for (unsigned int i = 0; i < count; i++)
	{
		if (Chars[i].TranslatedPic)
		{
			FFontChar1 *pic = static_cast<FFontChar1 *>(Chars[i].TranslatedPic);
			if (pic)
			{
				pic->SetSourceRemap(nullptr); // Force the FFontChar1 to return the same pixels as the base texture
				RecordTextureColors(pic, usedcolors);
			}
		}
	}

	ActiveColors = SimpleTranslation (usedcolors, PatchRemap, identity, Luminosity);

	for (unsigned int i = 0; i < count; i++)
	{
		if(Chars[i].TranslatedPic)
			static_cast<FFontChar1 *>(Chars[i].TranslatedPic)->SetSourceRemap(PatchRemap);
	}

	BuildTranslations (Luminosity.Data(), identity, &TranslationParms[TranslationType][0], ActiveColors, nullptr);
}

//==========================================================================
//
// FFont :: FFont - default constructor
//
//==========================================================================

FFont::FFont ()
{
	FontName = NAME_None;
	Cursor = '_';
	noTranslate = false;
	uint8_t pp = 0;
	for (auto &p : PatchRemap) p = pp++;
}

//==========================================================================
//
// FFont :: FixXMoves
//
// If a font has gaps in its characters, set the missing characters'
// XMoves to either SpaceWidth or the unaccented or uppercase variant's
// XMove. Missing XMoves must be initialized with INT_MIN beforehand.
//
//==========================================================================

void FFont::FixXMoves()
{
	for (int i = 0; i <= LastChar - FirstChar; ++i)
	{
		if (Chars[i].XMove == INT_MIN)
		{
			// Try an uppercase character.
			if (myislower(i + FirstChar))
			{
				int upper = upperforlower[FirstChar + i];
				if (upper >= FirstChar && upper <= LastChar )
				{
					Chars[i].XMove = Chars[upper - FirstChar].XMove;
					continue;
				}
			}
			// Try an unnaccented character.
			int noaccent = stripaccent(i + FirstChar);
			if (noaccent != i + FirstChar)
			{
				noaccent -= FirstChar;
				if (noaccent >= 0)
				{
					Chars[i].XMove = Chars[noaccent].XMove;
					continue;
				}
			}
			Chars[i].XMove = SpaceWidth;
		}
		if (Chars[i].OriginalPic)
		{
			int ofs = Chars[i].OriginalPic->GetTopOffset();
			if (ofs > Displacement) Displacement = ofs;
		}
	}
}


