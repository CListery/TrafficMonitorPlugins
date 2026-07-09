#include "pch.h"
#include "StockDef.h"
#include <utilities/yyjson/yyjson.h>

namespace STOCK
{
    std::tuple<wxString, wxString> difference(const wxString &n1, const wxString &n2,
                                              const int decimals_1 = 0, const int decimals_2 = 0)
    {
        int n1_decimals;
        int n2_decimals;
        if (decimals_1 != 0)
        {
            n1_decimals = decimals_1;
        }
        else
        {
            n1_decimals = 2;
        }

        if (decimals_2 != 0)
        {
            n2_decimals = decimals_2;
        }
        else if (decimals_1 != 0)
        {
            n2_decimals = decimals_1;
        }
        else
        {
            n2_decimals = 2;
        }

        std::string change, percent;

        double num_1 = UtilStringHlp::parseDouble(n1);
        double num_2 = UtilStringHlp::parseDouble(n2);
        if (UtilStringHlp::isValidNum(num_1) && UtilStringHlp::isValidNum(num_2))
        {
            double sub = num_1 - num_2;
            double percentVal = sub * 100.0 / num_2;

            change = UtilStringHlp::toFixed(sub, n1_decimals);
            percent = UtilStringHlp::toFixed(percentVal, n2_decimals) + "%";

            if (sub > 0)
            {
                change = "+" + change;
                percent = "+" + percent;
            }
        }
        else
        {
            change = "0.00";
            percent = "0.00%";
        }

        return {change, percent};
    }

    wxString f29(const wxString &p61, const wxString &p62)
    {
        wxString v49;
        switch (UtilStringHlp::parseInt(p62))
        {
        case 1:
            if (UtilStringHlp::parseInt(p61) != 0)
            {
                v49 = "停牌";
            }
            break;
        case 2:
            v49 = "未上市";
            break;
        case 3:
            v49 = "退市";
            break;
        default:
            v49 = "";
        }
        return v49;
    }

    wxString f43(size_t p195, size_t p196)
    {
        if (p195 == p196)
        {
            return "";
        }
        else
        {
            if (p195 - p196 > 0)
            {
                return "+";
            }
            else
            {
                return "";
            }
        }
    }

    wxString f24(wxString &p55)
    {
        if (p55 == "富时100指数")
        {
            return "英国富时100";
        }
        else if (p55 == "道琼斯欧元区斯托克50指数")
        {
            return "欧Stoxx50";
        }
        else if (p55 == "澳大利亚标准普尔200指数")
        {
            return "澳ASX200";
        }
        else if (p55 == "FTSE/JSE 南非40指数")
        {
            return "南非JSE40";
        }
        else
        {
            return p55;
        }
    }

    void LStockData::LoadByRealtimeData(const wxString &code, const wxString &raw_data)
    {
        this->code = code;
        this->market = GetMarketByCode(code);
        this->name = code + ": " + UtilResHlp.StringRes(IDS_LOADING);

        wxArrayString latest_data_arr = UtilStringHlp::split(raw_data, ",");

        if (this->market == "A" || this->market == "SI")
        {
            StockObj(code, latest_data_arr);
        }
        else if (this->market == "US")
        {
            USStockObj(code, latest_data_arr);
        }
        else if (this->market == "HF")
        {
            FuturesObj(code, latest_data_arr);
        }
        else if (this->market == "NF")
        {
            NffuturesObj(code, latest_data_arr);
        }
        else if (this->market == "DINIW")
        {
            DINIWObj(code, latest_data_arr);
        }
        else if (this->market == "FX")
        {
            ForexObj(code, latest_data_arr);
        }
        else if (this->market == "B")
        {
            GlobalObj(code, latest_data_arr);
        }
        else if (this->market == "SB")
        {
            SBstockObj(code, latest_data_arr);
        }
        else if (this->market == "HK")
        {
            HkstockObj(code, latest_data_arr);
        }
        else if (this->market == "RTHK")
        {
            HkstockObj(code, latest_data_arr);
        }
        else if (this->market == "GlobalBD")
        {
            GlobalBDObj(code, latest_data_arr);
        }
        else if (this->market == "BT")
        {
            BitcoinObj(code, latest_data_arr);
        }
        else if (this->market == "FUND")
        {
            FUNDObj(code, latest_data_arr);
        }
        else if (this->market == "ZNB")
        {
            ZNBGBObj(code, latest_data_arr);
        }
        else if (this->market == "BLOCK")
        {
            BlockIndexObj(code, latest_data_arr);
        }
        else if (this->market == "LSE")
        {
            UKLSEObj(code, latest_data_arr);
        }
        else if (this->market == "GOODS")
        {
            GOODSObj(code, latest_data_arr);
        }
        else if (this->market == "MSCI")
        {
            MSCIObj(code, latest_data_arr);
        }
        else if (this->market == "GlobalBD")
        {
            GlobalBDObj(code, latest_data_arr);
        }
        else
        {
            // TODO: reset
        }
    }

    void LStockData::LoadBySearchData(const wxString &raw_data)
    {
        wxArrayString codes = CommonUtils::StringHelper::split(raw_data, ",");

        type = codes[1];
        name = codes[6];
        if (name.empty())
        {
            name = codes[4];
        }

        wxString p129 = codes[2];
        wxString p130 = codes[3];
        wxString p131 = codes[4];
        if (p131.empty())
        {
            p131 = codes[6];
        }

        if (type == "11" || type == "12" || type == "81" || type == "120" || type == "203" || type == "204")
        {
            code = p130;
        }
        else if (type == "202" || type == "201")
        {
            code = "f_" + p129;
        }
        else if (type == "73")
        {
            code = "sb" + p129;
        }
        else if (type == "31" || type == "32" || type == "33")
        {
            code = "hk" + p129.Upper();
        }
        else if (type == "41")
        {
            code = p129;
            code.Replace(".", "$");
            code = "gb_" + code;
        }
        else if (type == "71")
        {
            code = "fx_s" + p129;
            if (p129.Lower() == "diniw")
            {
                code = "DINIW";
            }
            if (p129.Lower() == "btcokcoin")
            {
                code = "btc_btcokcoin";
            }
            if (p129.Lower() == "btcbitstamp")
            {
                code = "btc_btcbitstamp";
            }
        }
        else if (type == "86")
        {
            code = "hf_" + p129.Upper();
        }
        else if (type == "85" || type == "87" || type == "88")
        {
            code = "nf_" + p129.Upper();
            if (code == "nf_10000001")
            {
                code = "nf_" + p129.Upper() + "|50期权";
            }
        }
        else if (type == "77")
        {
            code = "sw2_" + p129 + "|" + p131;
        }
        else if (type == "78")
        {
            code = "chgn_" + p129 + "|" + p131;
        }
        else if (type == "79")
        {
            code = "diyu_" + p129 + "|" + p131;
        }
        else if (type == "100")
        {
            code = "znb_" + p129.Upper();
        }
        else if (type == "103")
        {
            code = "lse_" + p129;
        }
        else if (type == "114")
        {
            code = p129;
            code.Replace("globalbd", "");
            code = "globalbd_" + code;
        }

        LoadUrl(codes[2], codes[3]);

        market = STOCK::GetMarketByCode(code);
    }

    void LStockData::LoadUrl(const wxString &t, const wxString &s)
    {
        static const wxString URL_PREFIX_nfutures = "https://gu.sina.cn/ft/hq/nf.php?symbol=";
        static const wxString URL_PREFIX_hfutures = "https://gu.sina.cn/ft/hq/hf.php?symbol=";
        static const wxString URL_PREFIX_usstock = "https://gu.sina.cn/us/hq/quotes.php?code=";
        static const wxString URL_PREFIX_forex = "https://gu.sina.cn/fx/hq/quotes.php?code=";
        static const wxString URL_PREFIX_stock = "https://quotes.sina.cn/hs/company/quotes/view/";
        static const wxString URL_PREFIX_bond = "https://gu.sina.cn/bd/hq/quotes.php?symbol=";
        static const wxString URL_PREFIX_sanban = "https://gu.sina.cn/tm/hq/quotes.php?code=";
        static const wxString URL_PREFIX_hkstock = "https://quotes.sina.cn/hk/company/quotes/view/";
        static const wxString URL_PREFIX_bitcoin = "https://stocks.sina.cn/bit/detail?wh=";
        static const wxString URL_PREFIX_center = "https://gu.sina.cn/m/#/stock/blockdetail?id=";
        static const wxString URL_PREFIX_fund = "https://stocks.sina.cn/fund/?code=";
        static const wxString URL_PREFIX_znb = "https://quotes.sina.cn/global/hq/quotes.php?code=";
        static const wxString URL_PREFIX_uk = "https://quotes.sina.cn/lse/hq/quotes.php?symbol=";
        static const wxString URL_PREFIX_globalbd = "https://quotes.sina.cn/bd/hq/globalbd.php?symbol=";

        if (type == "11" || type == "12" || type == "203" || type == "204")
        {
            url = URL_PREFIX_stock + s + "?from=nbsearchresult";
        }
        else if (type == "202" || type == "201")
        {
            url = URL_PREFIX_fund + t + "&from=nbsearchresult";
        }
        else if (type == "31" || type == "32" || type == "33")
        {
            url = URL_PREFIX_hkstock + t.Upper() + "?from=nbsearchresult";
        }
        else if (type == "41")
        {
            url = URL_PREFIX_usstock + t + "&from=nbsearchresult";
        }
        else if (type == "77")
        {
            url = URL_PREFIX_center + "sw2_" + t + "&from=nbsearchresult";
        }
        else if (type == "78")
        {
            url = URL_PREFIX_center + "chgn_" + t + "&from=nbsearchresult";
        }
        else if (type == "79")
        {
            url = URL_PREFIX_center + "diyu_" + t + "&from=nbsearchresult";
        }
        else if (type == "102")
        {
            url = URL_PREFIX_stock + s + "?from=nbsearchresult";
        }
        else if (type == "71")
        {
            url = URL_PREFIX_forex + t + "&from=nbsearchresult";
        }
        else if (type == "73")
        {
            url = URL_PREFIX_sanban + t + "&from=nbsearchresult";
        }
        else if (type == "81" || type == "120")
        {
            url = URL_PREFIX_bond + s + "&from=nbsearchresult";
        }
        else if (type == "85" || type == "87" || type == "88")
        {
            url = URL_PREFIX_nfutures + t + "&from=nbsearchresult";
            if (s == "10000001")
            {
                url = "http://stocks.sina.cn/op/?vt=4&from=nbsearchresult";
            }
        }
        else if (type == "86")
        {
            url = URL_PREFIX_hfutures + t + "&from=nbsearchresult";
        }
        else if (type == "100")
        {
            url = URL_PREFIX_znb + t + "&from=nbsearchresult";
        }
        else if (type == "103")
        {
            url = URL_PREFIX_uk + t + "&from=nbsearchresult";
        }
        else if (type == "114")
        {
            url = URL_PREFIX_globalbd + "globalbd_" + t + "&from=nbsearchresult";
        }
    }

    void LStockData::StockObj(wxString p140, wxArrayString v124)
    {
        if (v124.GetCount() > 33)
        {
            v124.RemoveAt(33, v124.GetCount() - 33);
        }
        wxArrayString v111 = wxArrayString(v124);
        v111.push_back("");
        v111.push_back("");

        // 股票名称
        this->name = v111[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        bool v142 = (p140.find("sz15") == 0 || p140.find("sz16") == 0 || p140.find("sz18") == 0) //
                    || (p140.find("sh50") == 0 || p140.find("sh51") == 0 || p140.find("sh52") == 0);

        int v89 = 2;

        double v74 = UtilStringHlp::parseDouble(v111[3]) == 0 ? UtilStringHlp::parseDouble(v111[2]) : UtilStringHlp::parseDouble(v111[3]);
        double v133 = UtilStringHlp::selectValid(UtilStringHlp::parseDouble(v124[11]), UtilStringHlp::parseDouble(v124[21]), UtilStringHlp::parseDouble(v124[2]));
        if (UtilStringHlp::TimeToSecond(v111[31]) < UtilStringHlp::TimeToSecond("09:29") && UtilStringHlp::TimeToSecond(v111[31]) > UtilStringHlp::TimeToSecond("09:14"))
        {
            v74 = v133;
        }

        if (v142)
        {
            open = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v111[1], 3), UtilStringHlp::toFixed(v111[1], v89));
            prevclose = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v111[2], 3), UtilStringHlp::toFixed(v111[2], v89));
            wxString v74_1 = UtilStringHlp::toFixed(v74, 3);
            wxString v74_2 = UtilStringHlp::toFixed(v74, v89);
            price = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v74_1, 3), UtilStringHlp::toFixed(v74_2, v89));
            high = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v111[4], 3), UtilStringHlp::toFixed(v111[4], v89));
            low = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v111[5], 3), UtilStringHlp::toFixed(v111[5], v89));
        }
        else
        {
            open = UtilStringHlp::toFixed(v111[1], v89);
            prevclose = UtilStringHlp::toFixed(v111[2], v89);
            wxString v74_1 = UtilStringHlp::toFixed(v74, 3);
            wxString v74_2 = UtilStringHlp::toFixed(v74, v89);
            price = UtilStringHlp::toFixed(v74_2, v89);
            high = UtilStringHlp::toFixed(v111[4], v89);
            low = UtilStringHlp::toFixed(v111[5], v89);
        }
        totalVolume_i = UtilStringHlp::toFixed(v111[8]);
        totalAmount_i = UtilStringHlp::toFixed(v111[9]);

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS = difference(wxString::FromDouble(v74), v111[2], v89);
        wxString v76 = std::get<0>(vS);
        wxString v75 = std::get<1>(vS);
        auto vS2 = difference(wxString::FromDouble(v74), v111[2], 3);
        wxString v136 = std::get<0>(vS2);
        if (v142)
        {
            if (!v136.empty())
            {
                displayPrice = v136;
            }
            else
            {
                displayPrice = v76;
            }
        }
        else
        {
            displayPrice = v76;
        }
        displayFluctuation = v75;
    }

    void LStockData::USStockObj(wxString p161, wxArrayString v190)
    {
        // 股票名称
        this->name = v190[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v190[5]);
        prevclose = UtilStringHlp::toFixed(v190[26]);
        price = UtilStringHlp::toFixed(v190[1], 3);
        high = UtilStringHlp::toFixed(v190[6]);
        low = UtilStringHlp::toFixed(v190[7]);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS6 = difference(v190[1], v190[26], 3, 2);
        wxString v184 = std::get<0>(vS6);
        wxString v183 = std::get<1>(vS6);

        displayPrice = v184;
        displayFluctuation = v183;
    }

    void LStockData::FuturesObj(wxString p168, wxArrayString v211)
    {
        // 股票名称
        this->name = v211[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        int v208 = p168.find("EC") != wxString::npos             //
                           || p168.find("BP") != wxString::npos  //
                           || p168.find("JY") != wxString::npos  //
                           || p168.find("CD") != wxString::npos  //
                           || p168.find("SF") != wxString::npos  //
                           || p168.find("DXF") != wxString::npos //
                       ? 4
                       : (UtilStringHlp::parseInt(v211[0]) < 100 ? 3 : 2);

        open = UtilStringHlp::toFixed(v211[8], v208);
        prevclose = UtilStringHlp::toFixed(v211[7], v208);
        price = UtilStringHlp::toFixed(v211[0], v208);
        high = UtilStringHlp::toFixed(v211[4], v208);
        low = UtilStringHlp::toFixed(v211[5], v208);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS1 = difference(v211[0], v211[7], v208, 2);
        wxString v206 = std::get<0>(vS1);
        wxString v205 = std::get<1>(vS1);

        displayPrice = v206;
        displayFluctuation = v205;
    }

    void LStockData::NffuturesObj(wxString p170, wxArrayString v217)
    {
        // 股票名称
        this->name = v217[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v217[2]);
        prevclose = UtilStringHlp::toFixed(v217[10]);
        price = UtilStringHlp::toFixed(v217[8]);
        high = UtilStringHlp::toFixed(v217[3]);
        low = UtilStringHlp::toFixed(v217[4]);

        // totalVolume: UtilStringHlp::parseInt(v217[14]) || "--",
        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS1 = difference(v217[8], v217[10]);
        wxString v213 = std::get<0>(vS1);
        wxString v212 = std::get<1>(vS1);

        displayPrice = v213;
        displayFluctuation = v212;
    }

    void LStockData::DINIWObj(wxString p175, wxArrayString v235)
    {
        // 股票名称
        this->name = v235[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v235[5]);
        prevclose = UtilStringHlp::toFixed(v235[3], 4);
        price = UtilStringHlp::toFixed(v235[8], 4);
        high = UtilStringHlp::toFixed(v235[6]);
        low = UtilStringHlp::toFixed(v235[7]);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS7 = difference(v235[8], v235[3], 4);
        wxString v232 = std::get<0>(vS7);
        wxString v231 = std::get<1>(vS7);

        displayPrice = v232;
        displayFluctuation = v231;
    }

    void LStockData::ForexObj(wxString p177, wxArrayString v243)
    {
        // 股票名称
        this->name = v243[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        int vLN4 = UtilStringHlp::parseInt(v243[8]) < 10 ? 4 : UtilStringHlp::parseInt(v243[8]) < 100 ? 3
                                                                                                      : 2;

        open = UtilStringHlp::toFixed(v243[5], vLN4);
        prevclose = UtilStringHlp::toFixed(v243[3], vLN4);
        price = UtilStringHlp::toFixed(v243[8], vLN4);
        high = UtilStringHlp::toFixed(v243[6], vLN4);
        low = UtilStringHlp::toFixed(v243[7], vLN4);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS8 = difference(v243[8], v243[3], 4);
        wxString v237 = std::get<0>(vS8);
        wxString v236 = std::get<1>(vS8);

        displayPrice = v237;
        displayFluctuation = v236;
    }

    void LStockData::GlobalObj(wxString p179, wxArrayString v247)
    {
        // 股票名称
        this->name = v247[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = NAN;
        prevclose = NAN;
        price = UtilStringHlp::toFixed(v247[1]);
        high = NAN;
        low = NAN;

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        displayFluctuation = UtilStringHlp::parseDouble(v247[3]) > 0 ? "+" + v247[3] + "%" : v247[3] + "%";
        displayPrice = UtilStringHlp::parseDouble(v247[2]) > 0 ? "+" + v247[2] : v247[2];
    }

    void LStockData::SBstockObj(wxString p187, wxArrayString v274)
    {
        // 股票名称
        this->name = v274[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v274[1]);
        prevclose = UtilStringHlp::toFixed(v274[2]);
        price = UtilStringHlp::toFixed(v274[3]);
        high = UtilStringHlp::toFixed(v274[4]);
        low = UtilStringHlp::toFixed(v274[5]);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        auto vS11 = difference(v274[3], v274[2]);
        wxString v270 = std::get<0>(vS11);
        wxString v269 = std::get<1>(vS11);
        displayFluctuation = v269;
        displayPrice = v270;
    }

    void LStockData::HkstockObj(wxString p183, wxArrayString v259)
    {
        // 股票名称
        this->name = v259[1];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        double v268;

        if (UtilStringHlp::TimeToSecond(v259[18]) >= UtilStringHlp::TimeToSecond("09:00:00") && UtilStringHlp::TimeToSecond(v259[18]) < UtilStringHlp::TimeToSecond("09:20:00"))
        {
            v268 = UtilStringHlp::toFixed(v259[9], 3);
        }
        else
        {
            v268 = UtilStringHlp::toFixed(v259[6], 3);
        }

        if (!UtilStringHlp::isValidNum(v268))
        {
            v268 = UtilStringHlp::toFixed(v259[3], 3);
        }

        open = UtilStringHlp::toFixed(v259[2], 3);
        prevclose = UtilStringHlp::toFixed(v259[3], 3);
        price = v268;
        high = UtilStringHlp::toFixed(v259[4], 3);
        low = UtilStringHlp::toFixed(v259[5], 3);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        auto vS10 = difference(wxString::FromDouble(v268), v259[3], 3);
        wxString v254 = std::get<0>(vS10);
        wxString v253 = std::get<1>(vS10);
        displayFluctuation = v253;
        displayPrice = v254;
    }

    void LStockData::BitcoinObj(wxString p189, wxArrayString v280)
    {
        // 股票名称
        this->name = v280[1];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v280[5], 4);
        prevclose = UtilStringHlp::toFixed(v280[3], 4);
        price = UtilStringHlp::toFixed(v280[8], 4);
        high = UtilStringHlp::toFixed(v280[6], 4);
        low = UtilStringHlp::toFixed(v280[7], 4);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        auto vS12 = difference(v280[8], v280[3], 4);
        wxString v276 = std::get<0>(vS12);
        wxString v275 = std::get<1>(vS12);
        displayFluctuation = v275;
        displayPrice = v276;
    }

    void LStockData::FUNDObj(wxString p192, wxArrayString v281)
    {
        this->name = v281[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = NAN;
        prevclose = UtilStringHlp::parseDouble(v281[3]);
        price = UtilStringHlp::toFixed(v281[1], 4);
        high = NAN;
        low = NAN;

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        double v283 = UtilStringHlp::parseDouble(v281[1]);
        double v284 = UtilStringHlp::parseDouble(v281[3]);
        if (UtilStringHlp::isValidNum(v283) && UtilStringHlp::isValidNum(v284))
        {
            // TODO: 自定义小数点位数
            wxString vF43 = f43(v283, v284);
            displayFluctuation = vF43 + UtilStringHlp::toFixed((v283 / v284 - 1) * 100, 2) + "%";
            displayPrice = UtilStringHlp::toFixed(v283 - v284, 3);
        }
    }

    void LStockData::ZNBGBObj(wxString p181, wxArrayString v252)
    {
        this->name = f24(v252[0]);
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v252[8]);
        prevclose = UtilStringHlp::toFixed(v252[9]);
        price = UtilStringHlp::toFixed(v252[1]);
        high = UtilStringHlp::toFixed(v252[10]);
        low = UtilStringHlp::toFixed(v252[11]);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        wxString v248 = UtilStringHlp::parseDouble(v252[3]) > 0 ? "+" + v252[3] + "%" : v252[3] + "%";
        wxString v249 = UtilStringHlp::parseDouble(v252[2]) > 0 ? "+" + v252[2] : v252[2];

        displayFluctuation = v248;
        displayPrice = v249;
    }

    void LStockData::BlockIndexObj(wxString p181, wxArrayString v164)
    {
        this->name = v164[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v164[1], 2);
        prevclose = UtilStringHlp::toFixed(v164[2], 2);
        price = UtilStringHlp::toFixed(v164[3], 2);
        high = UtilStringHlp::toFixed(v164[4], 2);
        low = UtilStringHlp::toFixed(v164[5], 2);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        auto vS4 = difference(v164[3], v164[2], 2);
        wxString v167 = std::get<0>(vS4);
        wxString v168 = std::get<1>(vS4);
        displayFluctuation = v168;
        displayPrice = v167;
    }

    void LStockData::UKLSEObj(wxString p202, wxArrayString v293)
    {
        this->name = f24(v293[0]);
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v293[3], 3);
        prevclose = UtilStringHlp::toFixed(v293[5], 3);
        price = UtilStringHlp::toFixed(v293[1], 3);
        high = UtilStringHlp::toFixed(v293[2], 3);
        low = UtilStringHlp::toFixed(v293[4], 3);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        double vParseFloat3 = UtilStringHlp::parseDouble(v293[5]);
        double v298 = UtilStringHlp::parseDouble(v293[1]);
        if (!UtilStringHlp::isValidNum(v298))
        {
            v298 = vParseFloat3;
        }

        auto vS13 = difference(UtilStringHlp::toFixed(v298), UtilStringHlp::toFixed(vParseFloat3), 3);
        wxString v287 = std::get<0>(vS13);
        wxString v286 = std::get<1>(vS13);
        displayFluctuation = v286;
        displayPrice = v287;
    }

    void LStockData::GOODSObj(wxString p166, wxArrayString v204)
    {
        // 股票名称
        this->name = v204[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        int v201 = UtilStringHlp::parseInt(v204[0]) < 100 ? 3 : 2;

        open = UtilStringHlp::toFixed(v204[8], v201);
        prevclose = UtilStringHlp::toFixed(v204[7], v201);
        price = UtilStringHlp::toFixed(v204[0], v201);
        high = UtilStringHlp::toFixed(v204[4], v201);
        low = UtilStringHlp::toFixed(v204[5], v201);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        auto vS1 = difference(v204[0], v204[7], v201);
        wxString v199 = std::get<0>(vS1);
        auto vS2 = difference(v204[0], v204[7]);
        wxString v198 = std::get<1>(vS2);

        displayPrice = v199;
        displayFluctuation = v198;
    }

    void LStockData::MSCIObj(wxString p205, wxArrayString v300)
    {
        // 股票名称
        this->name = v300[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v300[21], 2);
        prevclose = UtilStringHlp::toFixed(v300[22], 2);
        price = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v300[4], 2), UtilStringHlp::toFixed(v300[22], 2));
        high = UtilStringHlp::toFixed(v300[19], 2);
        low = UtilStringHlp::toFixed(v300[20], 2);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        double v301 = UtilStringHlp::parseDouble(v300[22]);
        double v302 = UtilStringHlp::parseDouble(v300[4]);
        if (!UtilStringHlp::isValidNum(v302))
        {
            v302 = v301;
        }
        double v303 = v302 - v301;
        double v304 = v303 * 100 / v301;

        displayPrice = UtilStringHlp::toFixed(v303, 2);
        displayFluctuation = UtilStringHlp::toFixed(v304, 2) + "%";
    }

    void LStockData::GlobalBDObj(wxString p207, wxArrayString v308)
    {
        // 股票名称
        this->name = v308[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        double vNumber = UtilStringHlp::toFixed(v308[2], 3);
        double v309 = UtilStringHlp::toFixed(v308[3], 3);
        if (!UtilStringHlp::isValidNum(v309))
        {
            v309 = vNumber;
        }
        double v310 = v309 - vNumber;
        double v311 = v310 * 100 / abs(vNumber);

        open = UtilStringHlp::toFixed(v308[1]);
        prevclose = vNumber;
        price = v309;
        high = UtilStringHlp::toFixed(v308[4]);
        low = UtilStringHlp::toFixed(v308[5]);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        // TODO: 自定义小数点位数
        displayPrice = UtilStringHlp::toFixed(v310, 3);
        displayFluctuation = UtilStringHlp::toFixed(v311, 3) + "%";
    }

    wxString GetMarketByCode(const wxString &code)
    {
        if (code.find("sh") == 0 || code.find("sz") == 0 || (code.size() > 2 && code.find("bj") == 0))
        {
            return "A";
        }
        else if (code.find("gb_") == 0 || code.find("usr_") == 0)
        {
            return "US";
        }
        else if (code.find("hf_") == 0)
        {
            return "HF";
        }
        else if (code.find("nf_") == 0)
        {
            return "NF";
        }
        else if (code.find("si") == 0)
        {
            return "SI";
        }
        else if (code.find("DINIW") != wxString::npos || code.find("XAGUSD") != wxString::npos || code.find("XAUUSD") != wxString::npos || code.find("EURI") != wxString::npos)
        {
            return "DINIW";
        }
        else if (code.find("fx_s") == 0)
        {
            return "FX";
        }
        else if (code.find("b_") == 0)
        {
            return "B";
        }
        else if (code.find("lse_") == 0)
        {
            return "LSE";
        }
        else if (code.find("gds_") == 0)
        {
            return "GOODS";
        }
        else if (code.find("znb_") == 0)
        {
            return "ZNB";
        }
        else if (code.find("hk") == 0)
        {
            return "HK";
        }
        else if (code.find("sb") == 0)
        {
            return "SB";
        }
        else if (code.find("btc_") == 0)
        {
            return "BT";
        }
        else if (code.find("f_") == 0)
        {
            return "FUND";
        }
        else if (code.find("msci_") == 0)
        {
            return "MSCI";
        }
        else if (code.find("rt_") == 0)
        {
            return "RTHK";
        }
        else if (code.size() > 7 && (code.find("hy") == 0 || code.find("gn") == 0 || code.find("dy") == 0))
        {
            return "BLOCK";
        }
        else if (code.find("globalbd_") == 0)
        {
            return "GlobalBD";
        }
        else
        {
            return "NODATA";
        }
    }

    std::unordered_set<int> GetValidPeriodTypeValues()
    {
        return {
            static_cast<int>(LStockPeriodType::TIMELINE),
            static_cast<int>(LStockPeriodType::MIN1),
            static_cast<int>(LStockPeriodType::MIN5),
            static_cast<int>(LStockPeriodType::MIN15),
            static_cast<int>(LStockPeriodType::MIN30),
            static_cast<int>(LStockPeriodType::HOUR1),
            static_cast<int>(LStockPeriodType::DAY),
            static_cast<int>(LStockPeriodType::WEEK),
            static_cast<int>(LStockPeriodType::MONTH),
            static_cast<int>(LStockPeriodType::YEAR)};
    }

    LStockPeriodType IntToStockPeriodType(int val)
    {
        auto valid = GetValidPeriodTypeValues();
        if (valid.count(val))
        {
            return static_cast<LStockPeriodType>(val);
        }
        return LStockPeriodType::UNKNOWN;
    }

    void LStockData::LoadByConfig(const wxString &raw_data)
    {
        wxArrayString cfg = UtilStringHlp::split(raw_data, ",");
        wxString copy_code = cfg[0];
        copy_code.Replace(CFG_REPLACE_STR, ",");
        wxString copy_name = cfg[1];
        copy_name.Replace(CFG_REPLACE_STR, ",");
        wxString copy_type = cfg[2];
        copy_type.Replace(CFG_REPLACE_STR, ",");
        wxString copy_url = cfg[3];
        copy_url.Replace(CFG_REPLACE_STR, ",");

        code = copy_code;
        name = copy_name;
        type = copy_type;
        url = copy_url;
    }

    wxString LStockData::ToConfig() const
    {
        wxString copy_code = wxString(code);
        copy_code.Replace(",", CFG_REPLACE_STR, true);
        wxString copy_name = wxString(name);
        copy_name.Replace(",", CFG_REPLACE_STR, true);
        wxString copy_type = wxString(type);
        copy_type.Replace(",", CFG_REPLACE_STR, true);
        wxString copy_url = wxString(url);
        copy_url.Replace(",", CFG_REPLACE_STR, true);
        wxArrayString cfg;
        cfg.push_back(copy_code);
        cfg.push_back(copy_name);
        cfg.push_back(copy_type);
        cfg.push_back(copy_url);
        return UtilStringHlp::vectorJoinString(cfg, ",");
    }

    wxString LStockData::GetBridgData(LStockPeriodType type) const
    {
        auto pointDataIt = period_data_map.find(type);
        if (pointDataIt == period_data_map.end())
        {
            return wxEmptyString;
        }

        yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
        yyjson_mut_val *root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);

        yyjson_mut_val *timeRangeObj = yyjson_mut_obj(doc);

        //"start": "09:30:00",
        //"end" : "15:30:00",
        //"breakStart" : "11:30:00",
        //"breakEnd" : "13:00:00"

        // hkap: [["16:15", "18:30"]],
        // uk: [["8:00", "16:30"]],
        // repo: [["9:30", "11:30"], ["13:00", "15:30"]],

        // isSZRepo: function(e) {
        //     return /^sz1318\d{2}$/.test(e) ? "repo" : !1
        // },
        // isRepos: function(e) {
        //     return /^(sh204\d{3}|sz1318\d{2})$/.test(e)
        // },
        // isBJ: function(e) {
        //     return /^bjd{6}$/.test(e)
        // },
        // isGZ: function(e) {
        //     return /^sh(009|010|018)\d{3}$/.test(e) ? "bond" : /^sh11\d{4}$/.test(e) ? "bond" : !1
        // },
        // isBond: function(e) {
        //     return /^(sh204\d{3}|sz1318\d{2})$/.test(e) ? "bond" : /^sh020\d{3}$/.test(e) ? "bond" : /^sz108\d{3}$/.test(e) ? "bond" : /^sh(009|010|018)\d{3}$/.test(e) ? "bond" : /^sz10\d{4}$/.test(e) ? "bond" : /^sh(100|110|112|113)\d{3}$/.test(e) ? "bond" : /^sz12\d{4}$/.test(e) ? "bond" : /^sh11\d{4}$/.test(e) ? "bond" : /^sh(105|120|129|139)\d{3}$/.test(e) ? "bond" : /^sz11\d{4}$/.test(e) ? "bond" : /^sh12\d{4}$/.test(e) ? "bond" : !1
        // },
        // tp: function(e) {
        //     return toString.call(e).slice(8, -1)
        // },
        wxString start, end, breakStart, breakEnd;
        if (this->market == "GlobalBD")
        {
            // globalbd: [["08:00", "23:59"], ["00:00", "07:59"]],
            start = "00:00";
            end = "23:59";
            breakStart = "";
            breakEnd = "";
        }
        else if (this->market == "A" || this->market == "SI")
        {
            // si: [["09:30", "11:30"], ["13:01", "15:00"]],
            // cnplate: [["09:30", "11:30"], ["13:01", "15:00"]],
            // cn: [["09:30", "11:30"], ["13:01", "15:00"]],
            start = "09:30";
            end = "15:00";
            breakStart = "11:30";
            breakEnd = "13:01";
        }
        else if (this->market == "US")
        {
            // us: [["9:30", "16:00"]],
            start = "9:30";
            end = "16:00";
            breakStart = "";
            breakEnd = "";
        }
        else if (this->market == "HK")
        {
            // hk: [["09:30", "11:59"], ["13:00", "16:00"]],
            start = "09:30";
            end = "16:00";
            breakStart = "11:59";
            breakEnd = "13:00";
        }
        else if (this->market == "LSE")
        {
            // LSE: [["8:00", "16:30"]],
            start = "8:00";
            end = "16:30";
            breakStart = "";
            breakEnd = "";
        }
        else if (this->market == "GOODS")
        {
            // goods: [["20:00", "23:59"], ["00:00", "02:29"], ["09:00", "15:30"]],
            start = "00:00";
            end = "23:59";
            breakStart = "02:29,15:30";
            breakEnd = "09:00,20:00";
        }
        else if (this->market == "MSCI")
        {
            // msci: [["07:00", "23:59"], ["00:00", "06:00"]],
            start = "00:00";
            end = "23:59";
            breakStart = "06:00";
            breakEnd = "07:00";
        }
        else
        {
            return wxEmptyString;
        }
        yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "start", start.ToUTF8());
        yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "end", end.ToUTF8());
        yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "breakStart", breakStart.ToUTF8());
        yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "breakEnd", breakEnd.ToUTF8());

        yyjson_mut_val *newestObj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_real(doc, newestObj, "open", this->open);
        yyjson_mut_obj_add_real(doc, newestObj, "prevclose", this->prevclose);
        yyjson_mut_obj_add_real(doc, newestObj, "price", this->price);
        yyjson_mut_obj_add_real(doc, newestObj, "high", this->high);
        yyjson_mut_obj_add_real(doc, newestObj, "low", this->low);

        yyjson_mut_val *pointsArr = yyjson_mut_arr(doc);
        for (wxSharedPtr<STOCK::LStockPeriodDataBase> item : pointDataIt->second)
        {
            //"avg_p": "5.05",
            //"m" : "09:30:00",
            //"p" : "5.05",
            //"tot_v" : "8163761",
            //"v" : "8163761"
            yyjson_mut_val *itemObj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_real(doc, itemObj, "avg_p", item->averagePrice);
            yyjson_mut_obj_add_strcpy(doc, itemObj, "m", item->time.ToUTF8());
            yyjson_mut_obj_add_real(doc, itemObj, "p", item->price);
            yyjson_mut_obj_add_int(doc, itemObj, "tot_v", item->accumulationVolume);
            yyjson_mut_obj_add_int(doc, itemObj, "v", item->volume);
            yyjson_mut_arr_append(pointsArr, itemObj);
        }

        yyjson_mut_obj_add_val(doc, root, "time_range", timeRangeObj);
        yyjson_mut_obj_add_val(doc, root, "newest", newestObj);
        yyjson_mut_obj_add_val(doc, root, "points", pointsArr);

        size_t jsonLen = 0;
        char *jsonStr = yyjson_mut_write(doc, 0, &jsonLen);

        wxString result = wxEmptyString;
        if (jsonStr && jsonLen > 0)
        {
            result = wxString::FromUTF8(jsonStr, jsonLen);
            free(jsonStr);
        }
        
        yyjson_mut_doc_free(doc);

        return result;
    }

    wxSharedPtr<LStockData> LStockListVM::GetRowData(const wxDataViewItem &item)
    {
        unsigned int row = GetRow(item);

        if (row >= m_row_data.size())
            return wxSharedPtr<LStockData>(nullptr);

        return m_row_data[row];
    }

    void LStockListVM::GetValueByRow(wxVariant &variant, unsigned int row, unsigned int col) const
    {
        if (row >= m_row_data.size())
        {
            variant = wxEmptyString;
            return;
        }
        auto data = m_row_data[row];
        switch (col)
        {
        case Col_MarketText:
            variant = data->GetDisplayMarket();
            break;
        case Col_NameText:
            variant = data->name;
            break;
        case Col_CodeText:
            variant = data->code;
            break;
        }
    }

    bool LStockListVM::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr &attr) const
    {
        switch (col)
        {
        case Col_MarketText:
            attr.SetColour(wxColour(*wxLIGHT_GREY));
            break;
        case Col_NameText:
            attr.SetColour(wxColour(*wxBLACK));
            return true;
        case Col_CodeText:
            attr.SetColour(wxColour(*wxBLACK));
            return true;
        }

        return false;
    }

    bool LStockListVM::SetValueByRow(const wxVariant &variant, unsigned int row, unsigned int col)
    {
        //    if (row >= m_row_data.size())
        //    {
        //        return false;
        //    }
        //    LStockRowData *data = m_row_data[row];
        //    switch (col)
        //    {
        //    case Col_NameText:
        //        data->item_name = variant.GetString();
        //        break;
        //    case Col_CodeText:
        //        data->item_code = variant.GetString();
        //        break;
        //    }
        //    return true;
        return false;
    }

    bool LStockPeriodTimelineData::LoadJsonIterator(wxString json_data, yyjson_arr_iter *iter, yyjson_doc **out_doc)
    {
        if (iter == nullptr || out_doc == nullptr)
        {
            return false;
        }
        *out_doc = nullptr;

        wxScopedCharBuffer utf8Buf = json_data.utf8_str();
        const char *utf8Str = utf8Buf.data();
        size_t utf8Len = utf8Buf.length();
        if (utf8Str == nullptr || utf8Len == 0)
        {
            return false;
        }

        yyjson_doc *doc = yyjson_read(utf8Str, utf8Len, 0);
        if (doc == nullptr)
        {
            return false;
        }

        yyjson_val *root = yyjson_doc_get_root(doc);
        if (root == nullptr || !yyjson_is_obj(root))
        {
            yyjson_doc_free(doc);
            return false;
        }

        yyjson_val *result = yyjson_obj_get(root, "result");
        if (result == nullptr || !yyjson_is_obj(result))
        {
            yyjson_doc_free(doc);
            return false;
        }

        yyjson_val *data = yyjson_obj_get(result, "data");
        if (data != nullptr && yyjson_is_arr(data))
        {
            yyjson_arr_iter_init(data, iter);
            *out_doc = doc; // 传出doc指针，调用者用完必须手动释放
            return true;
        }

        yyjson_doc_free(doc);
        return false;
    }

    void LStockPeriodTimelineData::DispatchHandle(yyjson_val *item)
    {
        time = UtilJsonHlp::GetString(item, "m");
        volume = UtilJsonHlp::GetLL(item, "v");
        price = UtilJsonHlp::GetDouble(item, "p");
        averagePrice = UtilJsonHlp::GetDouble(item, "avg_p");
        accumulationVolume = UtilJsonHlp::GetLL(item, "tot_v");
    }

}
