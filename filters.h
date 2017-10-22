#ifndef _FILTERS_H_
#define _FILTERS_H_

class CGenericFilter
{
public:
    
    CGenericFilter (float_precision dWidth) : m_dWidth (dWidth) {}
    virtual ~CGenericFilter() {}

    float_precision GetWidth()                   { return m_dWidth; }
    void   SetWidth (float_precision dWidth)     { m_dWidth = dWidth; }

    virtual float_precision Filter (float_precision dVal) = 0;

protected:

    #define FILTER_PI  float_precision (3.1415926535897932384626433832795)
    #define FILTER_2PI float_precision (2.0 * 3.1415926535897932384626433832795)
    #define FILTER_4PI float_precision (4.0 * 3.1415926535897932384626433832795)

    float_precision  m_dWidth;
};

class CBoxFilter : public CGenericFilter
{
public:

    CBoxFilter (float_precision dWidth = float_precision(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBoxFilter() {}

    float_precision Filter (float_precision dVal) { return (fabs((double)dVal) <= m_dWidth ? 1.0 : 0.0); }
};

class CBilinearFilter : public CGenericFilter
{
public:

    CBilinearFilter (float_precision dWidth = float_precision(1.0)) : CGenericFilter(dWidth) {}
    virtual ~CBilinearFilter() {}

    float_precision Filter (float_precision dVal) 
        {
            dVal = fabs((double)dVal); 
            return (dVal < m_dWidth ? m_dWidth - dVal : 0.0); 
        }
};

class CGaussianFilter : public CGenericFilter
{
public:

    CGaussianFilter (float_precision dWidth = float_precision(3.0)) : CGenericFilter(dWidth) {}
    virtual ~CGaussianFilter() {}

    float_precision Filter (float_precision dVal) 
        {
            if (fabs ((double)dVal) > m_dWidth) 
            {
                return 0.0;
            }
            return exp (-dVal * dVal / 2.0) / sqrt ((double)FILTER_2PI); 
        }
};

class CHammingFilter : public CGenericFilter
{
public:

    CHammingFilter (float_precision dWidth = float_precision(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CHammingFilter() {}

    float_precision Filter (float_precision dVal) 
        {
            if (fabs ((double)dVal) > m_dWidth) 
            {
                return 0.0; 
            }
            float_precision dWindow = 0.54 + 0.46 * cos ((double)FILTER_2PI * dVal); 
            float_precision dSinc = (dVal == 0) ? 1.0 : sin ((double)FILTER_PI * dVal) / (FILTER_PI * dVal); 
            return dWindow * dSinc;
        }
};

class CBlackmanFilter : public CGenericFilter
{
public:

    CBlackmanFilter (float_precision dWidth = float_precision(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBlackmanFilter() {}

    float_precision Filter (float_precision dVal) 
        {
            if (fabs ((double)dVal) > m_dWidth) 
            {
                return 0.0; 
            }
            float_precision dN = 2.0 * m_dWidth + 1.0; 
            return 0.42 + 0.5 * cos (FILTER_2PI * dVal / ( dN - 1.0 )) + 
                   0.08 * cos (FILTER_4PI * dVal / ( dN - 1.0 )); 
        }
};
 
 
#endif  // _FILTERS_H_
