#include "pch.h"
#include "StockDef.h"
#include <utilities/yyjson/yyjson.h>
#include <regex>

namespace STOCK
{
    std::tuple<wxString, wxString> difference(const wxString &n1, const wxString &n2, const int decimals_1 = 0, const int decimals_2 = 2)
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

    /**
     * HQ_DataApps.js#DataCenter.prototype._handle
     */
    void LStockData::LoadByRealtimeData(const wxString &code, const wxString &raw_data)
    {
        this->code = code;
        this->name = code + ": " + UtilResHlp.StringRes(IDS_LOADING);

        wxArrayString latest_data_arr = UtilStringHlp::split(raw_data, ",");

        switch (getMarket(code))
        {
        case GetMarketType::GetMarketType_A:
            StockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_US:
            USStockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_HF:
            FuturesObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_NF:
            NffuturesObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_SI:
            StockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_DINIW:
            DINIWObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_FX:
            ForexObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_B:
            GlobalObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_LSE:
            UKLSEObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_GOODS:
            GOODSObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_ZNB:
            ZNBGBObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_HK:
            HkstockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_SB:
            SBstockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_BT:
            BitcoinObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_FUND:
            FUNDObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_MSCI:
            MSCIObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_RTHK:
            HkstockObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_BLOCK:
            BlockIndexObj(code, latest_data_arr);
            break;
        case GetMarketType::GetMarketType_GlobalBD:
            GlobalBDObj(code, latest_data_arr);
            break;
        default:
            break;
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

        int v89 = this->decimals;

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

        auto vS = difference(wxString::FromDouble(v74), v111[2], v89);
        wxString v76 = std::get<0>(vS);
        wxString v75 = std::get<1>(vS);
        auto vS2 = difference(wxString::FromDouble(v74), v111[2], 3);
        wxString v136 = std::get<0>(vS2);
        if (v142)
        {
            if (!v136.empty())
            {
                changePrice = v136;
            }
            else
            {
                changePrice = v76;
            }
        }
        else
        {
            changePrice = v76;
        }
        changeFluctuation = v75;
    }

    void LStockData::USStockObj(wxString p161, wxArrayString v190)
    {
        // 股票名称
        this->name = v190[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v190[5], this->decimals);
        prevclose = UtilStringHlp::toFixed(v190[26], this->decimals);
        price = UtilStringHlp::toFixed(v190[1], this->decimals);
        high = UtilStringHlp::toFixed(v190[6], this->decimals);
        low = UtilStringHlp::toFixed(v190[7], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        auto vS6 = difference(v190[1], v190[26], this->decimals, 2);
        wxString v184 = std::get<0>(vS6);
        wxString v183 = std::get<1>(vS6);

        changePrice = v184;
        changeFluctuation = v183;
    }

    void LStockData::FuturesObj(wxString p168, wxArrayString v211)
    {
        // 股票名称
        this->name = v211[13];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        // int v208 = p168.find("EC") != wxString::npos             //
        //                    || p168.find("BP") != wxString::npos  //
        //                    || p168.find("JY") != wxString::npos  //
        //                    || p168.find("CD") != wxString::npos  //
        //                    || p168.find("SF") != wxString::npos  //
        //                    || p168.find("DXF") != wxString::npos //
        //                ? 4
        //                : (UtilStringHlp::parseInt(v211[0]) < 100 ? 3 : 2);
        const int v208 = this->decimals;

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

        changePrice = v206;
        changeFluctuation = v205;
    }

    void LStockData::NffuturesObj(wxString p170, wxArrayString v217)
    {
        // 股票名称
        this->name = v217[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v217[2], this->decimals);
        prevclose = UtilStringHlp::toFixed(v217[10], this->decimals);
        price = UtilStringHlp::toFixed(v217[8], this->decimals);
        high = UtilStringHlp::toFixed(v217[3], this->decimals);
        low = UtilStringHlp::toFixed(v217[4], this->decimals);

        // totalVolume: UtilStringHlp::parseInt(v217[14]) || "--",
        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        auto vS1 = difference(v217[8], v217[10], this->decimals);
        wxString v213 = std::get<0>(vS1);
        wxString v212 = std::get<1>(vS1);

        changePrice = v213;
        changeFluctuation = v212;
    }

    void LStockData::DINIWObj(wxString p175, wxArrayString v235)
    {
        // 股票名称
        this->name = v235[9];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v235[5], this->decimals);
        prevclose = UtilStringHlp::toFixed(v235[3], this->decimals);
        price = UtilStringHlp::toFixed(v235[8], this->decimals);
        high = UtilStringHlp::toFixed(v235[6], this->decimals);
        low = UtilStringHlp::toFixed(v235[7], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        auto vS7 = difference(v235[8], v235[3], this->decimals);
        wxString v232 = std::get<0>(vS7);
        wxString v231 = std::get<1>(vS7);

        changePrice = v232;
        changeFluctuation = v231;
    }

    void LStockData::ForexObj(wxString p177, wxArrayString v243)
    {
        // 股票名称
        this->name = v243[9];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        // int vLN4 = UtilStringHlp::parseInt(v243[8]) < 10 ? 4 : UtilStringHlp::parseInt(v243[8]) < 100 ? 3 : 2;
        const int vLN4 = this->decimals;

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

        auto vS8 = difference(v243[8], v243[3], vLN4);
        wxString v237 = std::get<0>(vS8);
        wxString v236 = std::get<1>(vS8);

        changePrice = v237;
        changeFluctuation = v236;
    }

    void LStockData::GlobalObj(wxString p179, wxArrayString v247)
    {
        // 股票名称
        this->name = f24(v247[0]);
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = NAN;
        prevclose = NAN;
        price = UtilStringHlp::toFixed(v247[1], this->decimals);
        high = NAN;
        low = NAN;

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        changeFluctuation = UtilStringHlp::parseDouble(v247[3]) > 0 ? "+" + v247[3] + "%" : v247[3] + "%";
        double diffPrice = UtilStringHlp::parseDouble(v247[2]);
        wxString diffPriceStr = UtilStringHlp::toFixed(diffPrice, this->decimals);
        changePrice = diffPrice > 0 ? "+" + diffPriceStr : diffPriceStr;
    }

    void LStockData::SBstockObj(wxString p187, wxArrayString v274)
    {
        // 股票名称
        this->name = v274[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v274[1], this->decimals);
        prevclose = UtilStringHlp::toFixed(v274[2], this->decimals);
        price = UtilStringHlp::toFixed(v274[3], this->decimals);
        high = UtilStringHlp::toFixed(v274[4], this->decimals);
        low = UtilStringHlp::toFixed(v274[5], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        // TODO: 自定义小数点位数
        auto vS11 = difference(v274[3], v274[2], this->decimals);
        wxString v270 = std::get<0>(vS11);
        wxString v269 = std::get<1>(vS11);
        changeFluctuation = v269;
        changePrice = v270;
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
            v268 = UtilStringHlp::toFixed(v259[9], this->decimals);
        }
        else
        {
            v268 = UtilStringHlp::toFixed(v259[6], this->decimals);
        }

        if (!UtilStringHlp::isValidNum(v268))
        {
            v268 = UtilStringHlp::toFixed(v259[3], this->decimals);
        }

        open = UtilStringHlp::toFixed(v259[2], this->decimals);
        prevclose = UtilStringHlp::toFixed(v259[3], this->decimals);
        price = v268;
        high = UtilStringHlp::toFixed(v259[4], this->decimals);
        low = UtilStringHlp::toFixed(v259[5], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        auto vS10 = difference(wxString::FromDouble(v268), v259[3], this->decimals);
        wxString v254 = std::get<0>(vS10);
        wxString v253 = std::get<1>(vS10);
        changeFluctuation = v253;
        changePrice = v254;
    }

    void LStockData::BitcoinObj(wxString p189, wxArrayString v280)
    {
        // 股票名称
        this->name = v280[9];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v280[5], this->decimals);
        prevclose = UtilStringHlp::toFixed(v280[3], this->decimals);
        price = UtilStringHlp::toFixed(v280[8], this->decimals);
        high = UtilStringHlp::toFixed(v280[6], this->decimals);
        low = UtilStringHlp::toFixed(v280[7], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        auto vS12 = difference(v280[8], v280[3], this->decimals);
        wxString v276 = std::get<0>(vS12);
        wxString v275 = std::get<1>(vS12);
        changeFluctuation = v275;
        changePrice = v276;
    }

    void LStockData::FUNDObj(wxString p192, wxArrayString v281)
    {
        this->name = v281[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = NAN;
        prevclose = UtilStringHlp::toFixed(v281[3], this->decimals);
        price = UtilStringHlp::toFixed(v281[1], this->decimals);
        high = NAN;
        low = NAN;

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        double v283 = UtilStringHlp::parseDouble(v281[1]);
        double v284 = UtilStringHlp::parseDouble(v281[3]);
        if (UtilStringHlp::isValidNum(v283) && UtilStringHlp::isValidNum(v284))
        {
            wxString vF43 = f43(v283, v284);
            changeFluctuation = vF43 + UtilStringHlp::toFixed((v283 / v284 - 1) * 100, 2) + "%";
            changePrice = UtilStringHlp::toFixed(v283 - v284, this->decimals);
        }
    }

    void LStockData::ZNBGBObj(wxString p181, wxArrayString v252)
    {
        this->name = f24(v252[0]);
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v252[8], this->decimals);
        prevclose = UtilStringHlp::toFixed(v252[9], this->decimals);
        price = UtilStringHlp::toFixed(v252[1], this->decimals);
        high = UtilStringHlp::toFixed(v252[10], this->decimals);
        low = UtilStringHlp::toFixed(v252[11], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        wxString v248 = UtilStringHlp::parseDouble(v252[3]) > 0 ? "+" + v252[3] + "%" : v252[3] + "%";

        double diffPrice = UtilStringHlp::parseDouble(v252[2]);
        wxString diffPriceStr = UtilStringHlp::toFixed(diffPrice, this->decimals);

        changeFluctuation = v248;
        changePrice = diffPrice > 0 ? "+" + diffPriceStr : diffPriceStr;
    }

    void LStockData::BlockIndexObj(wxString p181, wxArrayString v164)
    {
        this->name = v164[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v164[1], this->decimals);
        prevclose = UtilStringHlp::toFixed(v164[2], this->decimals);
        price = UtilStringHlp::toFixed(v164[3], this->decimals);
        high = UtilStringHlp::toFixed(v164[4], this->decimals);
        low = UtilStringHlp::toFixed(v164[5], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        auto vS4 = difference(v164[3], v164[2], this->decimals);
        wxString v167 = std::get<0>(vS4);
        wxString v168 = std::get<1>(vS4);
        changeFluctuation = v168;
        changePrice = v167;
    }

    void LStockData::UKLSEObj(wxString p202, wxArrayString v293)
    {
        this->name = f24(v293[0]);
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v293[3], this->decimals);
        prevclose = UtilStringHlp::toFixed(v293[5], this->decimals);
        price = UtilStringHlp::toFixed(v293[1], this->decimals);
        high = UtilStringHlp::toFixed(v293[2], this->decimals);
        low = UtilStringHlp::toFixed(v293[4], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        priceLimit = NAN;

        double vParseFloat3 = UtilStringHlp::parseDouble(v293[5]);
        double v298 = UtilStringHlp::parseDouble(v293[1]);
        if (!UtilStringHlp::isValidNum(v298))
        {
            v298 = vParseFloat3;
        }

        auto vS13 = difference(UtilStringHlp::toFixed(v298), UtilStringHlp::toFixed(vParseFloat3), this->decimals);
        wxString v287 = std::get<0>(vS13);
        wxString v286 = std::get<1>(vS13);
        changeFluctuation = v286;
        changePrice = v287;
    }

    void LStockData::GOODSObj(wxString p166, wxArrayString v204)
    {
        // 股票名称
        this->name = v204[13];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        // int v201 = UtilStringHlp::parseInt(v204[0]) < 100 ? 3 : 2;
        const int v201 = this->decimals;

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

        auto vS1 = difference(v204[0], v204[7], v201);
        wxString v199 = std::get<0>(vS1);
        auto vS2 = difference(v204[0], v204[7]);
        wxString v198 = std::get<1>(vS2);

        changePrice = v199;
        changeFluctuation = v198;
    }

    void LStockData::MSCIObj(wxString p205, wxArrayString v300)
    {
        // 股票名称
        this->name = v300[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        open = UtilStringHlp::toFixed(v300[21], this->decimals);
        prevclose = UtilStringHlp::toFixed(v300[22], this->decimals);
        price = UtilStringHlp::selectValid(UtilStringHlp::toFixed(v300[4], this->decimals), UtilStringHlp::toFixed(v300[22], this->decimals));
        high = UtilStringHlp::toFixed(v300[19], this->decimals);
        low = UtilStringHlp::toFixed(v300[20], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        double v301 = UtilStringHlp::parseDouble(v300[22]);
        double v302 = UtilStringHlp::parseDouble(v300[4]);
        if (!UtilStringHlp::isValidNum(v302))
        {
            v302 = v301;
        }
        double v303 = v302 - v301;
        double v304 = v303 * 100 / v301;

        changePrice = UtilStringHlp::toFixed(v303, this->decimals);
        changeFluctuation = UtilStringHlp::toFixed(v304, 2) + "%";
    }

    void LStockData::GlobalBDObj(wxString p207, wxArrayString v308)
    {
        // 股票名称
        this->name = v308[0];
        if (this->name.empty())
        {
            this->name = UtilResHlp.StringRes(IDS_LOAD_FAIL);
        }

        double vNumber = UtilStringHlp::toFixed(v308[2], this->decimals);
        double v309 = UtilStringHlp::toFixed(v308[3], this->decimals);
        if (!UtilStringHlp::isValidNum(v309))
        {
            v309 = vNumber;
        }
        double v310 = v309 - vNumber;
        double v311 = v310 * 100 / abs(vNumber);

        open = UtilStringHlp::toFixed(v308[1], this->decimals);
        prevclose = vNumber;
        price = v309;
        high = UtilStringHlp::toFixed(v308[4], this->decimals);
        low = UtilStringHlp::toFixed(v308[5], this->decimals);

        totalVolume_i = NAN;
        totalAmount_i = NAN;

        Price upperLimit = abs(high - prevclose);
        Price lowerLimit = abs(low - prevclose);

        priceLimit = max(upperLimit, lowerLimit);

        changePrice = UtilStringHlp::toFixed(v310, this->decimals);
        changeFluctuation = UtilStringHlp::toFixed(v311, 3) + "%";
    }

    /**
     * sf_sdk.js#market
     */
    MarketType market(const wxString &code)
    {
        const std::string s = std::string(code.ToUTF8());

        static const std::regex reRepo(R"(^(sh204\d{3}|sz1318\d{2})$)");
        static const std::regex reSI(R"(^si\w+$)");
        static const std::regex reCNStock(R"(^s[hz]\d{6}$)");
        static const std::regex reBJ(R"(^bj\w+)");
        static const std::regex reGN(R"(^(GN|gn\d{7})$)");
        static const std::regex reHY(R"(^(HY|hy\d{7})$)");
        static const std::regex reDY(R"(^(DY|dy\d{7})$)");
        static const std::regex reCNI(R"(^s[hz]\d{6}_i$)");
        static const std::regex reSBOTC(R"(^sb[48]\d{5}$)");
        static const std::regex reOTC48(R"(^[48]\d{5}$)");
        static const std::regex reOTCPrefix(R"(^otc_\d{6}$)");
        static const std::regex reBTC(R"(^btc_\w+)");
        static const std::regex reUS(R"(^gb_.+$)");
        static const std::regex reHKPreIPO(R"(^(hk|rt_hk)\w+_preipo$)");
        static const std::regex reHK(R"(^(hk|rt_hk)\w+)");
        static const std::regex reHF(R"(^hf_\w+)");
        static const std::regex reGlobalBD(R"(^globalbd_.+$)");
        static const std::regex reLSE(R"(^lse_.+$)");
        static const std::regex reNF(R"(^nf_\w+)");
        static const std::regex reGOODS(R"(^gds_\w+)");
        static const std::regex reFund(R"(^(f_\d{6}|fu_\d{6}|pwbfbyd_\d{6}|pwbfbjd_\d{6}|pwbfbnd_\d{6}|ljjz_\d{6}|dwjz_\d{6}|lshb_\d{6})$)");
        static const std::regex reCNOption(R"(^CON_OP_\w+)");
        static const std::regex rePMOption(R"(^P_OP_\w+)");
        static const std::regex reGlobalIndex(R"(^znb_\w+)");
        static const std::regex reForex(R"(^fx_.+$)");
        static const std::regex reForexYT(R"(^(DINIW|USDCNY)$)");
        static const std::regex reCFF(R"(^CFF_RE_.+$)");
        static const std::regex reMSCI(R"(^msci_\w+)");
        static const std::regex reAllDigit(R"(^\d+$)");

        if (std::regex_match(s, reRepo))
        {
            return MarketType::MarketType_REPO;
        }
        else if (std::regex_match(s, reSI))
        {
            return MarketType::MarketType_SI;
        }
        else if (std::regex_match(s, reCNStock))
        {
            return MarketType::MarketType_SZ_SH;
        }
        else if (std::regex_match(s, reBJ))
        {
            return MarketType::MarketType_BJ;
        }
        else if (std::regex_match(s, reGN))
        {
            return MarketType::MarketType_GN;
        }
        else if (std::regex_match(s, reHY))
        {
            return MarketType::MarketType_HY;
        }
        else if (std::regex_match(s, reDY))
        {
            return MarketType::MarketType_DY;
        }
        else if (std::regex_match(s, reCNI))
        {
            return MarketType::MarketType_CNI;
        }
        else if (std::regex_match(s, reSBOTC))
        {
            return MarketType::MarketType_OTC;
        }
        else if (std::regex_match(s, reOTC48))
        {
            return MarketType::MarketType_OTC;
        }
        else if (std::regex_match(s, reOTCPrefix))
        {
            return MarketType::MarketType_OTC;
        }
        else if (std::regex_match(s, reBTC))
        {
            return MarketType::MarketType_BTC;
        }
        else if (std::regex_match(s, reUS))
        {
            return MarketType::MarketType_US;
        }
        else if (std::regex_match(s, reHKPreIPO))
        {
            return MarketType::MarketType_HKAP;
        }
        else if (std::regex_match(s, reHK))
        {
            return MarketType::MarketType_HK;
        }
        else if (std::regex_match(s, reHF))
        {
            return MarketType::MarketType_HF;
        }
        else if (std::regex_match(s, reGlobalBD))
        {
            return MarketType::MarketType_globalbd;
        }
        else if (std::regex_match(s, reLSE))
        {
            return MarketType::MarketType_LSE;
        }
        else if (std::regex_match(s, reNF))
        {
            return MarketType::MarketType_NF;
        }
        else if (std::regex_match(s, reGOODS))
        {
            return MarketType::MarketType_GOODS;
        }
        else if (std::regex_match(s, reFund))
        {
            return MarketType::MarketType_fund;
        }
        else if (std::regex_match(s, reCNOption))
        {
            return MarketType::MarketType_option_cn;
        }
        else if (std::regex_match(s, rePMOption))
        {
            return MarketType::MarketType_op_m;
        }
        else if (std::regex_match(s, reGlobalIndex))
        {
            return MarketType::MarketType_global_index;
        }
        else if (std::regex_match(s, reForex))
        {
            return MarketType::MarketType_forex;
        }
        else if (std::regex_match(s, reForexYT))
        {
            return MarketType::MarketType_forex_yt;
        }
        else if (std::regex_match(s, reCFF))
        {
            return MarketType::MarketType_CFF;
        }
        else if (std::regex_match(s, reMSCI))
        {
            return MarketType::MarketType_MSCI;
        }
        else if (std::regex_match(s, reAllDigit))
        {
            return MarketType::MarketType_NF;
        }
        else
        {
            return MarketType::MarketType_UNKNOWN;
        }
    }

    /**
     * HQ_DataApps.js#getMarket
     */
    GetMarketType getMarket(const wxString &code)
    {
        static const std::regex reBJ(R"(^bj\w+)");
        static const std::regex reBlock(R"((hy|gn|dy)\d{7})", std::regex_constants::icase);

        const std::string s = std::string(code.ToUTF8());

        if (code.StartsWith(wxT("sh")) || code.StartsWith(wxT("sz")) || std::regex_search(s, reBJ))
        {
            return GetMarketType::GetMarketType_A;
        }
        else if (code.StartsWith(wxT("gb_")) || code.StartsWith(wxT("usr_")))
        {
            return GetMarketType::GetMarketType_US;
        }
        else if (code.StartsWith(wxT("hf_")))
        {
            return GetMarketType::GetMarketType_HF;
        }
        else if (code.StartsWith(wxT("nf_")))
        {
            return GetMarketType::GetMarketType_NF;
        }
        else if (code.StartsWith(wxT("si")))
        {
            return GetMarketType::GetMarketType_SI;
        }
        else if (code.Find(wxT("DINIW")) != wxNOT_FOUND ||
                 code.Find(wxT("XAGUSD")) != wxNOT_FOUND ||
                 code.Find(wxT("XAUUSD")) != wxNOT_FOUND ||
                 code.Find(wxT("EURI")) != wxNOT_FOUND)
        {
            return GetMarketType::GetMarketType_DINIW;
        }
        else if (code.StartsWith(wxT("fx_s")))
        {
            return GetMarketType::GetMarketType_FX;
        }
        else if (code.StartsWith(wxT("b_")))
        {
            return GetMarketType::GetMarketType_B;
        }
        else if (code.StartsWith(wxT("lse_")))
        {
            return GetMarketType::GetMarketType_LSE;
        }
        else if (code.StartsWith(wxT("gds_")))
        {
            return GetMarketType::GetMarketType_GOODS;
        }
        else if (code.StartsWith(wxT("znb_")))
        {
            return GetMarketType::GetMarketType_ZNB;
        }
        else if (code.StartsWith(wxT("hk")))
        {
            return GetMarketType::GetMarketType_HK;
        }
        else if (code.StartsWith(wxT("sb")))
        {
            return GetMarketType::GetMarketType_SB;
        }
        else if (code.StartsWith(wxT("btc_")))
        {
            return GetMarketType::GetMarketType_BT;
        }
        else if (code.StartsWith(wxT("f_")))
        {
            return GetMarketType::GetMarketType_FUND;
        }
        else if (code.StartsWith(wxT("msci_")))
        {
            return GetMarketType::GetMarketType_MSCI;
        }
        else if (code.StartsWith(wxT("rt_")))
        {
            return GetMarketType::GetMarketType_RTHK;
        }
        else if (std::regex_search(s, reBlock))
        {
            return GetMarketType::GetMarketType_BLOCK;
        }
        else if (code.StartsWith(wxT("globalbd_")))
        {
            return GetMarketType::GetMarketType_GlobalBD;
        }
        else
        {
            return GetMarketType::GetMarketType_UNKNOWN;
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

    bool LStockData::LoadByConfig(const wxString &raw_data)
    {
        wxArrayString cfg = UtilStringHlp::split(raw_data, ",");
        if (cfg.empty() || cfg.size() < 4)
        {
            return false;
        }
        wxString copy_code = cfg[0];
        copy_code.Replace(CFG_REPLACE_STR, ",");
        wxString copy_name = cfg[1];
        copy_name.Replace(CFG_REPLACE_STR, ",");
        wxString copy_type = cfg[2];
        copy_type.Replace(CFG_REPLACE_STR, ",");
        wxString copy_url = cfg[3];
        copy_url.Replace(CFG_REPLACE_STR, ",");

        wxString copy_decimals = wxEmptyString;
        if (cfg.size() > 4)
        {
            copy_decimals = cfg[4];
            copy_decimals.Replace(CFG_REPLACE_STR, ",");
        }

        code = copy_code;
        name = copy_name;
        type = copy_type;
        url = copy_url;
        if (copy_decimals.empty())
        {
            decimals = DEFAULT_DECIMAL_PLACES;
        }
        else
        {
            if (!copy_decimals.ToUInt(&decimals))
            {
                decimals = DEFAULT_DECIMAL_PLACES;
            }
            if (decimals > MAX_DECIMAL_PLACES || decimals < MIN_DECIMAL_PLACES)
            {
                decimals = DEFAULT_DECIMAL_PLACES;
            }
        }
        return true;
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
        wxString copy_decimals = wxString::Format("%d", decimals);
        wxArrayString cfg;
        cfg.push_back(copy_code);
        cfg.push_back(copy_name);
        cfg.push_back(copy_type);
        cfg.push_back(copy_url);
        cfg.push_back(copy_decimals);
        return UtilStringHlp::vectorJoinString(cfg, ",");
    }

        wxString LStockData::GetBridgData(LStockPeriodType type) const
    {
        auto pointRawDataIt = period_raw_data_map.find(type);
        if (pointRawDataIt == period_raw_data_map.end())
        {
            return wxEmptyString;
        }

        yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
        yyjson_mut_val *root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);

        yyjson_mut_val *timeRangeObj = yyjson_mut_obj(doc);

        wxString start, end, breakStart, breakEnd = wxEmptyString;

        switch (market(this->code))
        {
        case MarketType::MarketType_REPO:
            // repo: [["9:30", "11:30"], ["13:00", "15:30"]],
            start = "09:30";
            end = "15:30";
            breakStart = "11:30";
            breakEnd = "13:00";
            break;
        case MarketType::MarketType_SZ_SH:
        case MarketType::MarketType_BJ:
        case MarketType::MarketType_GN:
        case MarketType::MarketType_HY:
        case MarketType::MarketType_SI:
        case MarketType::MarketType_CNI:
        case MarketType::MarketType_DY:
            // si: [["09:30", "11:30"], ["13:01", "15:00"]],
            // cnplate: [["09:30", "11:30"], ["13:01", "15:00"]],
            // cn: [["09:30", "11:30"], ["13:01", "15:00"]],
            start = "09:30";
            end = "15:00";
            breakStart = "11:30";
            breakEnd = "13:01";
            break;
        case MarketType::MarketType_OTC:
            break;
        case MarketType::MarketType_US:
            // us: [["9:30", "16:00"]],
            start = "9:30";
            end = "16:00";
            breakStart = "";
            breakEnd = "";
            break;
        case MarketType::MarketType_HKAP:
            // hkap: [["16:15", "18:30"]],
            start = "16:15";
            end = "18:30";
            breakStart = "";
            breakEnd = "";
            break;
        case MarketType::MarketType_HK:
            // hk: [["09:30", "11:59"], ["13:00", "16:00"]],
            start = "09:30";
            end = "16:00";
            breakStart = "11:59";
            breakEnd = "13:00";
            break;
        case MarketType::MarketType_HF:
            // hf: undefined,
            start = "00:00";
            end = "23:59";
            breakStart = "05:00";
            breakEnd = "06:00";
            break;
        case MarketType::MarketType_globalbd:
            // globalbd: [["08:00", "23:59"], ["00:00", "07:59"]],
            start = "00:00";
            end = "23:59";
            breakStart = "";
            breakEnd = "";
            break;
        case MarketType::MarketType_LSE:
        case MarketType::MarketType_BTC:
            // LSE: [["8:00", "16:30"]],
            // uk: [["8:00", "16:30"]],
            start = "8:00";
            end = "16:30";
            breakStart = "";
            breakEnd = "";
            break;
        case MarketType::MarketType_NF:
            // nf: undefined,
            break;
        case MarketType::MarketType_GOODS:
            // goods: [["20:00", "23:59"], ["00:00", "02:29"], ["09:00", "15:30"]],
            start = "00:00";
            end = "23:59";
            breakStart = "02:29,15:30";
            breakEnd = "09:00,20:00";
            break;
        case MarketType::MarketType_fund:
            break;
        case MarketType::MarketType_option_cn:
            break;
        case MarketType::MarketType_op_m:
            break;
        case MarketType::MarketType_global_index:
            break;
        case MarketType::MarketType_forex:
            break;
        case MarketType::MarketType_forex_yt:
            break;
        case MarketType::MarketType_CFF:
            break;
        case MarketType::MarketType_MSCI:
            // msci: [["07:00", "23:59"], ["00:00", "06:00"]],
            start = "00:00";
            end = "23:59";
            breakStart = "06:00";
            breakEnd = "07:00";
            break;
        default:
            break;
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

        yyjson_mut_obj_add_val(doc, root, "time_range", timeRangeObj);
        yyjson_mut_obj_add_val(doc, root, "newest", newestObj);
        yyjson_mut_obj_add_strcpy(doc, root, "raw_data", pointRawDataIt->second);
        yyjson_mut_obj_add_real(doc, root, "decimals", this->decimals);

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

    // wxString LStockData::GetBridgData(LStockPeriodType type) const
    // {
    //     auto pointDataIt = period_data_map.find(type);
    //     if (pointDataIt == period_data_map.end())
    //     {
    //         return wxEmptyString;
    //     }

    //     yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
    //     yyjson_mut_val *root = yyjson_mut_obj(doc);
    //     yyjson_mut_doc_set_root(doc, root);

    //     yyjson_mut_val *timeRangeObj = yyjson_mut_obj(doc);

    //     wxString start, end, breakStart, breakEnd = wxEmptyString;

    //     switch (market(this->code))
    //     {
    //     case MarketType::MarketType_REPO:
    //         // repo: [["9:30", "11:30"], ["13:00", "15:30"]],
    //         start = "09:30";
    //         end = "15:30";
    //         breakStart = "11:30";
    //         breakEnd = "13:00";
    //         break;
    //     case MarketType::MarketType_SZ_SH:
    //     case MarketType::MarketType_BJ:
    //     case MarketType::MarketType_GN:
    //     case MarketType::MarketType_HY:
    //     case MarketType::MarketType_SI:
    //     case MarketType::MarketType_CNI:
    //     case MarketType::MarketType_DY:
    //         // si: [["09:30", "11:30"], ["13:01", "15:00"]],
    //         // cnplate: [["09:30", "11:30"], ["13:01", "15:00"]],
    //         // cn: [["09:30", "11:30"], ["13:01", "15:00"]],
    //         start = "09:30";
    //         end = "15:00";
    //         breakStart = "11:30";
    //         breakEnd = "13:01";
    //         break;
    //     case MarketType::MarketType_OTC:
    //         break;
    //     case MarketType::MarketType_US:
    //         // us: [["9:30", "16:00"]],
    //         start = "9:30";
    //         end = "16:00";
    //         breakStart = "";
    //         breakEnd = "";
    //         break;
    //     case MarketType::MarketType_HKAP:
    //         // hkap: [["16:15", "18:30"]],
    //         start = "16:15";
    //         end = "18:30";
    //         breakStart = "";
    //         breakEnd = "";
    //         break;
    //     case MarketType::MarketType_HK:
    //         // hk: [["09:30", "11:59"], ["13:00", "16:00"]],
    //         start = "09:30";
    //         end = "16:00";
    //         breakStart = "11:59";
    //         breakEnd = "13:00";
    //         break;
    //     case MarketType::MarketType_HF:
    //         // hf: undefined,
    //         break;
    //     case MarketType::MarketType_globalbd:
    //         // globalbd: [["08:00", "23:59"], ["00:00", "07:59"]],
    //         start = "00:00";
    //         end = "23:59";
    //         breakStart = "";
    //         breakEnd = "";
    //         break;
    //     case MarketType::MarketType_LSE:
    //     case MarketType::MarketType_BTC:
    //         // LSE: [["8:00", "16:30"]],
    //         // uk: [["8:00", "16:30"]],
    //         start = "8:00";
    //         end = "16:30";
    //         breakStart = "";
    //         breakEnd = "";
    //         break;
    //     case MarketType::MarketType_NF:
    //         // nf: undefined,
    //         break;
    //     case MarketType::MarketType_GOODS:
    //         // goods: [["20:00", "23:59"], ["00:00", "02:29"], ["09:00", "15:30"]],
    //         start = "00:00";
    //         end = "23:59";
    //         breakStart = "02:29,15:30";
    //         breakEnd = "09:00,20:00";
    //         break;
    //     case MarketType::MarketType_fund:
    //         break;
    //     case MarketType::MarketType_option_cn:
    //         break;
    //     case MarketType::MarketType_op_m:
    //         break;
    //     case MarketType::MarketType_global_index:
    //         break;
    //     case MarketType::MarketType_forex:
    //         break;
    //     case MarketType::MarketType_forex_yt:
    //         break;
    //     case MarketType::MarketType_CFF:
    //         break;
    //     case MarketType::MarketType_MSCI:
    //         // msci: [["07:00", "23:59"], ["00:00", "06:00"]],
    //         start = "00:00";
    //         end = "23:59";
    //         breakStart = "06:00";
    //         breakEnd = "07:00";
    //         break;
    //     default:
    //         break;
    //     }

    //     yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "start", start.ToUTF8());
    //     yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "end", end.ToUTF8());
    //     yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "breakStart", breakStart.ToUTF8());
    //     yyjson_mut_obj_add_strcpy(doc, timeRangeObj, "breakEnd", breakEnd.ToUTF8());

    //     yyjson_mut_val *newestObj = yyjson_mut_obj(doc);
    //     yyjson_mut_obj_add_real(doc, newestObj, "open", this->open);
    //     yyjson_mut_obj_add_real(doc, newestObj, "prevclose", this->prevclose);
    //     yyjson_mut_obj_add_real(doc, newestObj, "price", this->price);
    //     yyjson_mut_obj_add_real(doc, newestObj, "high", this->high);
    //     yyjson_mut_obj_add_real(doc, newestObj, "low", this->low);

    //     yyjson_mut_val *pointsArr = yyjson_mut_arr(doc);
    //     for (wxSharedPtr<STOCK::LStockPeriodDataBase> item : pointDataIt->second)
    //     {
    //         //"avg_p": "5.05",
    //         //"m" : "09:30:00",
    //         //"p" : "5.05",
    //         //"tot_v" : "8163761",
    //         //"v" : "8163761"
    //         yyjson_mut_val *itemObj = yyjson_mut_obj(doc);
    //         yyjson_mut_obj_add_real(doc, itemObj, "avg_p", item->averagePrice);
    //         yyjson_mut_obj_add_strcpy(doc, itemObj, "m", item->time.ToUTF8());
    //         yyjson_mut_obj_add_real(doc, itemObj, "p", item->price);
    //         yyjson_mut_obj_add_int(doc, itemObj, "tot_v", item->accumulationVolume);
    //         yyjson_mut_obj_add_int(doc, itemObj, "v", item->volume);
    //         yyjson_mut_arr_append(pointsArr, itemObj);
    //     }

    //     yyjson_mut_obj_add_val(doc, root, "time_range", timeRangeObj);
    //     yyjson_mut_obj_add_val(doc, root, "newest", newestObj);
    //     yyjson_mut_obj_add_val(doc, root, "points", pointsArr);

    //     size_t jsonLen = 0;
    //     char *jsonStr = yyjson_mut_write(doc, 0, &jsonLen);

    //     wxString result = wxEmptyString;
    //     if (jsonStr && jsonLen > 0)
    //     {
    //         result = wxString::FromUTF8(jsonStr, jsonLen);
    //         free(jsonStr);
    //     }

    //     yyjson_mut_doc_free(doc);

    //     return result;
    // }

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
        case Col_DecimalsText:
            variant = (long)data->decimals;
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
        case Col_DecimalsText:
            attr.SetColour(wxColour(*wxBLACK));
            break;
        }

        return false;
    }

    bool LStockListVM::SetValueByRow(const wxVariant &variant, unsigned int row, unsigned int col)
    {
        if (row >= m_row_data.size())
        {
            return false;
        }
        auto data = m_row_data[row];
        switch (col)
        {
        case Col_DecimalsText:
            long val = variant.GetLong();
            if (val > MAX_DECIMAL_PLACES || val < MIN_DECIMAL_PLACES)
            {
                return false;
            }
            data->decimals = val;
            return true;
        }
        return false;
    }

    // bool LStockPeriodTimelineData::LoadJsonIterator(MarketType type, wxString json_data, yyjson_arr_iter *iter, yyjson_doc **out_doc)
    // {
    //     if (iter == nullptr || out_doc == nullptr)
    //     {
    //         return false;
    //     }
    //     *out_doc = nullptr;

    //     wxScopedCharBuffer utf8Buf = json_data.utf8_str();
    //     const char *utf8Str = utf8Buf.data();
    //     size_t utf8Len = utf8Buf.length();
    //     if (utf8Str == nullptr || utf8Len == 0)
    //     {
    //         return false;
    //     }

    //     yyjson_doc *doc = yyjson_read(utf8Str, utf8Len, 0);
    //     if (doc == nullptr)
    //     {
    //         return false;
    //     }

    //     yyjson_val *root = yyjson_doc_get_root(doc);
    //     if (root == nullptr || !yyjson_is_obj(root))
    //     {
    //         yyjson_doc_free(doc);
    //         return false;
    //     }

    //     switch (type)
    //     {
    //     case MarketType::MarketType_REPO:
    //         // {
    //         //     "result": {
    //         //         "status": {
    //         //             "code": 0,
    //         //             "msg": "MySql:lv1:success"
    //         //         },
    //         //         "data": [
    //         //             {
    //         //                 "m": "09:30:00",
    //         //                 "v": "477000",
    //         //                 "p": "7.19",
    //         //                 "avg_p": "7.19",
    //         //                 "tot_v": "477000"
    //         //             },
    //         //         ]
    //         //     }
    //         // }
    //     case MarketType::MarketType_BJ:
    //     case MarketType::MarketType_SI:
    //     case MarketType::MarketType_CNI:
    //     case MarketType::MarketType_DY:
    //     case MarketType::MarketType_GN:
    //     case MarketType::MarketType_HY:
    //     case MarketType::MarketType_SZ_SH:
    //         // {
    //         //     "result": {
    //         //         "status": {
    //         //             "code": 0,
    //         //             "msg": "MySql:lv1:success"
    //         //         },
    //         //         "data": [
    //         //             {
    //         //                 "m": "09:30:00",
    //         //                 "v": "5513800",
    //         //                 "p": "4.96",
    //         //                 "avg_p": "4.96",
    //         //                 "tot_v": "5513800"
    //         //             }
    //         //         ]
    //         //     }
    //         // }

    //         break;
    //     case MarketType::MarketType_OTC:
    //         break;
    //     case MarketType::MarketType_BTC:
    //         break;
    //     case MarketType::MarketType_US:
    //         // /*<script>location.href='//sina.com';</script>*/
    //         // var t1aapl=("09:30:00,4534779,331.859,332.4800;09:31:00,414814,331.867,331.5050;09:32:00,217974,331.886,332.8530;09:33:00,307562,331.917,332.9920;09:34:00,323419,331.977,333.4400;09:35:00,283580

    //         break;
    //     case MarketType::MarketType_HK:
    //     case MarketType::MarketType_HKAP:
    //         // {
    //         //     "result": {
    //         //         "status": {
    //         //             "code": 0
    //         //         },
    //         //         "data": [
    //         //             [
    //         //                 {
    //         //                     "date": "2026-07-17",
    //         //                     "prevclose": "70.45000",
    //         //                     "m": "09:30:00",
    //         //                     "price": "71.10000",
    //         //                     "volume": "83500",
    //         //                     "avg_p": "71.055"
    //         //                 },
    //         //                 {
    //         //                     "m": "09:31:00",
    //         //                     "price": "71.35000",
    //         //                     "volume": "122500",
    //         //                     "avg_p": "71.233"
    //         //                 },
    //         //             ]
    //         //         ]
    //         //     }
    //         // }

    //         break;
    //     case MarketType::MarketType_HF:
    //         // {
    //         //     "result": {
    //         //         "status": {
    //         //             "code": 0
    //         //         },
    //         //         "data": {
    //         //             "minLine_1d": [
    //         //                 [
    //         //                     "2026-07-17",
    //         //                     "3992.100",
    //         //                     "cme",
    //         //                     "",
    //         //                     "06:00",
    //         //                     "3981.260",
    //         //                     "0",
    //         //                     "0",
    //         //                     "3981.259",
    //         //                     "2026-07-17 06:00:00"
    //         //                 ],
    //         //                 [
    //         //                     "06:01",
    //         //                     "3982.021",
    //         //                     "0",
    //         //                     "0",
    //         //                     "3981.442",
    //         //                     "2026-07-17 06:01:00"
    //         //                 ],
    //         //             ]
    //         //         }
    //         //     }
    //         // }

    //         break;
    //     case MarketType::MarketType_globalbd:
    //         break;
    //     case MarketType::MarketType_LSE:
    //         break;
    //     case MarketType::MarketType_NF:
    //         break;
    //     case MarketType::MarketType_GOODS:
    //         break;
    //     case MarketType::MarketType_fund:
    //         break;
    //     case MarketType::MarketType_option_cn:
    //         break;
    //     case MarketType::MarketType_op_m:
    //         break;
    //     case MarketType::MarketType_global_index:
    //         break;
    //     case MarketType::MarketType_forex:
    //         break;
    //     case MarketType::MarketType_forex_yt:
    //         break;
    //     case MarketType::MarketType_CFF:
    //         break;
    //     case MarketType::MarketType_MSCI:
    //         break;
    //     default:
    //         break;
    //     }

    //     yyjson_val *result = yyjson_obj_get(root, "result");
    //     if (result == nullptr || !yyjson_is_obj(result))
    //     {
    //         yyjson_doc_free(doc);
    //         return false;
    //     }

    //     yyjson_val *data = yyjson_obj_get(result, "data");
    //     if (data != nullptr && yyjson_is_arr(data))
    //     {
    //         yyjson_arr_iter_init(data, iter);
    //         *out_doc = doc; // 传出doc指针，调用者用完必须手动释放
    //         return true;
    //     }

    //     yyjson_doc_free(doc);
    //     return false;
    // }

    // void LStockPeriodTimelineData::DispatchHandle(yyjson_val *item)
    // {
    //     time = UtilJsonHlp::GetString(item, "m");
    //     volume = UtilJsonHlp::GetLL(item, "v");
    //     price = UtilJsonHlp::GetDouble(item, "p");
    //     averagePrice = UtilJsonHlp::GetDouble(item, "avg_p");
    //     accumulationVolume = UtilJsonHlp::GetLL(item, "tot_v");
    // }

}
