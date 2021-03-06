#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include "YtcString.hpp"
#include "YtcCollection.hpp"
#define VAR(v) ","#v"="<<(v)


using namespace Ytc;
std::wostream& operator<<(std::wostream& stream, const WString& s)
{
    stream << s.Buffer();
    return stream;
}


template<typename T>
struct BasicTestCase
{
    T expectedValue;
    T actualValue;
};

static void TestYtcStringConcat()
{
    struct StringObjectPlusStringObject : BasicTestCase<WString>
    {
        StringObjectPlusStringObject(WString s1, WString s2, WString expected_value): lhs(std::move(s1)), rhs(std::move(s2)) 
        {
            expectedValue = std::move(expected_value);
        }
        WString lhs;
        WString rhs;
    };

    StringObjectPlusStringObject cases[] =
    {
        StringObjectPlusStringObject(L"", L"ytc", L"ytc"),
        StringObjectPlusStringObject(L"ytc", L"", L"ytc"),
        StringObjectPlusStringObject(L'y', L"tc", L"ytc"),
        StringObjectPlusStringObject(L"tc", L'y', L"tcy"),
        StringObjectPlusStringObject(L"yutuocheng ", L"is an excellent person!", L"yutuocheng is an excellent person!"),
        StringObjectPlusStringObject(L"", L"", L""),
        StringObjectPlusStringObject(L"abcdefghijklmnopqrstuvwxyz", L"0123456789876543210", L"abcdefghijklmnopqrstuvwxyz0123456789876543210"),
    };

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>Test Concat:\n";
    for (auto& c : cases)
    {
        c.actualValue = c.lhs + c.rhs;
        const auto& expectedValue = c.expectedValue;
        const auto& actualValue = c.actualValue;
        std::wcout << L"result=" << (expectedValue == actualValue) << VAR(expectedValue) << VAR(actualValue) << L'\n';
    }

    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>Test Append:\n";
    WString name = L"yutuocheng";
    name += L'a';
    assert(name == L"yutuochenga");
    name.Append(L"bc", 2);
    assert(name == L"yutuochengabc");
    name += L"yutuocheng";
    assert(name == L"yutuochengabcyutuocheng");

}

static void TestYtStringRemove()
{
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>Test Remove:\n";
    WString name = L"yutuocheng";
    std::wcout << name.Remove(2, 3) << std::endl;
    std::wcout << name.Remove(0, 2) << std::endl;
    std::wcout << name.Remove(0, 0) << std::endl;
    std::wcout << name.Remove(5, 5) << std::endl;
    std::wcout << name.Remove(0, 1) << std::endl;
    std::wcout << name.Remove(4, 1) << std::endl;

    WString tuo = L"tuo";
    auto pos = name.IndexOf(tuo);
    WString newString = name.Remove(pos, tuo.Length());
    std::wcout << newString << std::endl;
    assert(newString == L"yucheng");
    
    pos = name.LastIndexOf(L'u');
    assert(pos == 3);
}

static void TestYtcStringCOW()
{
    std::cout << __FUNCTION__ << std::endl;
    WString a('A', 256);
    std::wcout << VAR(a) << std::endl;
    WString b = a;
    std::wcout << VAR(b) << std::endl;

    b += L'C';
    std::wcout << VAR(b) << VAR(a) << std::endl;
    a = L'a';
    std::wcout << VAR(a) << std::endl;
}

static void TestYtcString()
{
    std::cout << __FUNCTION__"\n";
    const wchar_t* samples1[] =
    {
        //nullptr,
        L"",
        L"abc",
        L"abcdefghijklmn",
        L"abcdefghijklmnopqrst",
        L"qineeibreutbuebzvml;mefktnevee",
        L"applebanana*(8h3482h354g3lkngubvrnrkwamtbeugdlsnkengjkrbyvughfikcmlkdnatklebte,mhguiohnblkv rkybjlccccccccccccccz.ke000000000000000000feuibbbbbbbbbbubfubeje,nmbujgbvybaljfbejb",
        L"tcqwertyuioppmnbifdkebuckne,fnejtbeugonrknymrybrjx,"
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"\
        L"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongString",
        L"Ytc is excellent!",
    };

    std::cout << "\n>>>>>>>>>>>>>>>>>>>>>Length()\n";
    for (auto s : samples1)
    {
        WString str(s);
        uint32_t actual_length = str.Length();
        uint32_t expected_length = s ? wcslen(s) : 0;
        bool pass = actual_length == expected_length;
        std::cout << VAR(expected_length) << VAR(actual_length) << VAR(pass) << "\n";
    }

    std::cout << "\n>>>>>>>>>>>>>>>>>>>>Equation\n";
    for (auto s : samples1)
    {
        WString str(s);
        bool actual = s == str;
        bool expected_equal = s != nullptr;
        bool pass = actual == expected_equal;
        std::cout << VAR(expected_equal) << VAR(actual) << VAR(pass) << "\n";
    }

    std::cout << "\n>>>>>>>>>>>>>>>>>>>>>Comparision(by sorting them)\n";
    std::vector<WString> str_list;
    for (auto s : samples1)
    {
        str_list.emplace_back(s);
    }
    std::sort(str_list.begin(), str_list.end());
    std::cout << "size of list : " << str_list.size() << "\n";
    for (auto& s : str_list)
    {
        std::wcout << s.Buffer() << L"\n";
    }
    std::wcout.flush();

    std::cout << "\n>>>>>>>>>>>>>>>>>>>>>>SubString()\n";
    WString name = L"yutuocheng is an excellent person!";
    struct SubstringTestCases
    {
        uint32_t param_start;
        uint32_t param_length;
        WString expected_result;
    };
    SubstringTestCases substring_test_cases[] =
    {
        { 0, 0, L"" },
        { 0, 1, L"y"},
        { 1, 0, L""},
        { 11, 2, L"is"},
        { 0, -1, name },
        { 0, name.Length() - 1, L"yutuocheng is an excellent person" },
        { 11, 100, L"is an excellent person!"},
    };

    for (const auto& cs : substring_test_cases)
    {
        const WString& exepected_result = cs.expected_result;
        WString actual_result = name.SubString(cs.param_start, cs.param_length);
        bool pass = exepected_result == actual_result;
        std::wcout << VAR(exepected_result) << VAR(actual_result) << VAR(pass) << std::endl;
    }

    std::cout << "\n>>>>>>>>>>>>>>>>>>>>Test IndexOf()\n";

    TestYtcStringConcat();
    TestYtStringRemove();
    TestYtcStringCOW();
}

template<typename T>
static void Dump(IEnumerable<T>& collection)
{
    auto e = collection.GetEnumerator();
    while (e->MoveNext())
    {
        std::wcout << e->Current();
        std::wcout << L',';
    }

    std::wcout << std::endl;
}

static void TestList()
{
    List<int> l1;
    for (int i = 10; i > 0; --i)
    {
        l1.Add(i);
    }

    Dump(l1);

    {
        List<WString> list_str1;
        WString strings1[] = 
        {
            L"YU",
            L"TUO",
            L"CHENG",
            L"NIU",
            L"BI",
        };
        for (auto& s : strings1)
        {
            list_str1.Add(s);
        }

        auto list_str2 = list_str1;
        Dump(list_str1);
        Dump(list_str2);
        WString strings2[]=
        {
            L"SHI",
            L"ZHEN",
            L"DE",
            L"HAO",
        };

        for (int i = 0; i < std::size(strings2); ++i)
        {
            list_str1.Insert(i + 3, strings2[i]);
        }

        Dump(list_str1);
        list_str2 = list_str1;
        list_str2.RemoveAt(3);
        Dump(list_str2);
    }
}

int main()
{
    {
#ifdef _MSC_VER
        //MemLeakChecker checker;
#endif
        //TestYtcString();
        TestList();
    }
    std::cin.get();
    return 0;
}