#pragma once 
#include <optional>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <string.h>
#include <memory>


extern "C"
{
#include "whist/debug/plotter.h"
#include "whist/core/platform.h"
#include "whist/utils/clock.h"
}


#define ENABLE_WHIST_CHANGE true
namespace webrtc
{
    /*struct FieldTrialsView{
    };*/
    /*
    struct StructParametersParser {
    };*/
    template <typename T1,typename T2>
    using flat_map=std::map<T1,T2>;
};


#define absl std

namespace std
{
    inline bool StartsWith(string_view text,
                       string_view prefix) noexcept {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memcmp(text.data(), prefix.data(), prefix.size()) == 0);
    }
    template <typename T>
    unique_ptr<T> WrapUnique(T* a)
    {
        return unique_ptr<T>(a);
    }

};


namespace rtc
{
    /*
    inline double SafeClamp( double value, double low, double high)
    {
        value = std::max<double>(6.0f, value);
        value= std::min<double>(value, 600.0f);
        return value;
    }*/
    /*
    template <typename Dst, typename Src>
    //hacked version without dcheck
    inline constexpr Dst dchecked_cast(Src value) {
    return static_cast<Dst>(value);
    }*/
    inline std::string ToString(int a) {return std::to_string(a);}
    inline std::string ToString(unsigned int a) {return std::to_string(a);}
    inline std::string ToString(bool a) {return std::to_string(a);}
    inline std::string ToString(double a) {return std::to_string(a);}
    
    inline std::vector<absl::string_view> split(absl::string_view source, char delimiter) {
    std::vector<absl::string_view> fields;
    size_t last = 0;
    for (size_t i = 0; i < source.length(); ++i) {
        if (source[i] == delimiter) {
        fields.push_back(source.substr(last, i - last));
        last = i + 1;
        }
    }
    fields.push_back(source.substr(last));
    return fields;
    }
};


namespace my_logger
{
    struct MyCout {
    MyCout(std::ostream& os = std::cout) : os(os) {}
    struct A {
        A(std::ostream& r) : os(r), live(true) {}
        A(A& r) : os(r.os), live(true) {r.live=false;}
        A(A&& r) : os(r.os), live(true) {r.live=false;}
        ~A() { if(live) {os << std::endl;} }
        std::ostream& os;
        bool live;
    };
    std::ostream& os;
    };

    template <class T>
    MyCout::A operator<<(MyCout::A&& a, const T& t) {
    a.os << t;
    return a;
    }

    template<class T>
    MyCout::A operator<<(MyCout& m, const T& t) { return MyCout::A(m.os) << t; }

    extern MyCout my_cout;

#define  LS_VERBOSE "VERBOSE"
#define  LS_INFO "INFO"
#define  LS_WARNING "WARNING"
#define  LS_ERROR "ERROR"
#define  LS_NONE "NONE"
#define RTC_LOG(x) (my_logger::my_cout<<"["<<x<<"] ")
};





/*
namespace absl
{
    template<typename T>
    using optional=std::optional<T>;
    //using nullopt=std::nullopt;
    //const std::nullopt_t nullopt();
    inline constexpr std::nullopt_t nullopt();
};*/


/*
#define RTC_DCHECK(x) assert(x)
#define RTC_CHECK(x) assert(x)

#define RTC_DCHECK_GE(a, b) RTC_DCHECK(a>=b)
#define RTC_DCHECK_LE(a, b) RTC_DCHECK(a<=b)
#define RTC_DCHECK_GT(a, b) RTC_DCHECK(a>b)
#define RTC_DCHECK_LT(a, b) RTC_DCHECK(a<b)
#define RTC_DCHECK_EQ(a, b) RTC_DCHECK(a==b)

#define RTC_DCHECK_NOTREACHED() assert(0==1)
#define RTC_CHECK_NOTREACHED() assert(0==1)*/

#define RTC_EXPORT

#define BWE_TEST_LOGGING_PLOT(...)

#define whist_plotter_insert(...)

#define AdaptiveThresholdExperimentIsDisabled(...) false

#define RTC_DCHECK_RUNS_SERIALIZED(...)

#define RTC_HISTOGRAM_ENUMERATION(...)
#define RTC_HISTOGRAMS_COUNTS_100000(...)
#define RTC_HISTOGRAM_COUNTS(...)

#define LAST_SYSTEM_ERROR 0
