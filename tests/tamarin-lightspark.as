/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is [Open Source Virtual Machine.].
 *
 * The Initial Developer of the Original Code is
 * Adobe System Incorporated.
 * Portions created by the Initial Developer are Copyright (C) 2005-2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adobe AS3 Team
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * JavaScript shared functions file for running the tests in either
 * stand-alone JavaScript engine.  To run a test, first load this file,
 * then load the test script.
 */

// import flash.system in order to determine what we are being run in
import flash.system.*;

var completed = false;
var testcases;
var tc = 0;

// SECTION and VERSION cannot be 'var' because this file is included into
// test cases where SECTION and VERSION are redefined with 'var'; ASC
// barfs on that.  On the other hand, that means every test /must/
// define SECTION and VERSION, or the eval tests will not work because
// this file is not included into the test case when testing with eval.

SECTION = "";
VERSION = "";
var BUGNUMBER = "";
var STATUS = "STATUS: ";

//  constant strings

var GLOBAL = "[object global]";
var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";
var PACKAGELIST = "{public,$1$private}::";
var TYPEERROR = "TypeError: Error #";
var REFERENCEERROR = "ReferenceError: Error #";
var RANGEERROR = "RangeError: Error #";
var URIERROR = "URIError: Error #";
var EVALERROR = "EvalError: Error #";
var VERIFYERROR = "VerifyError: Error #";
var VERBOSE = true;
var ___failed:int = 0;
var ___numtest:int = 0;
var    DEBUG =  false;

// Was this compiled with -AS3?  Boolean Value.
var as3Enabled = ((new Namespace).valueOf != Namespace.prototype.valueOf);

function typeError( str ){
    return str.slice(0,TYPEERROR.length+4);
}

function referenceError( str ){
    return str.slice(0,REFERENCEERROR.length+4);
}
function rangeError( str ){
    return str.slice(0,RANGEERROR.length+4);
}
function uriError( str ){
    return str.slice(0,URIERROR.length+4);
}

function evalError( str ){
    return str.slice(0,EVALERROR.length+4);
}

function verifyError( str ){
    return str.slice(0,VERIFYERROR.length+4);
}

// wrapper for test cas constructor that doesn't require the SECTION
//  argument.


function AddTestCase( description, expect, actual ) {
    testcases[tc++] = new TestCase( SECTION, description, expect, actual );
}


//  TestCase constructor


function TestCase( n, d, e, a ) {
    this.name        = n;
    this.description = d;
    this.expect      = e;
    this.actual      = a;
    this.passed      = "";
    this.reason      = "";
    //this.bugnumber      = BUGNUMBER;

    this.passed = getTestCaseResult( this.expect, this.actual );
    if ( DEBUG ) {
        writeLineToLog( "added " + this.description );
    }
}


//  Set up test environment.


function startTest() {
    // print out bugnumber
    /*if ( BUGNUMBER ) {
            writeLineToLog ("BUGNUMBER: " + BUGNUMBER );
    }*/

    testcases = new Array();
    tc = 0;
}

function checkReason(passed) {
    var reason;
    if (passed == 'true') {
    reason = "";
    } else if (passed == 'false') {
    reason = "wrong value";
    } else if (passed == 'type error') {
    reason = "type error";
    }
    return reason;
}


function test(... rest:Array) {

    if( rest.length == 0 ){
        // no args sent, use default test
        for ( tc=0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );
            testcases[tc].reason += checkReason(testcases[tc].passed);
        }
    } else {
        // we need to use a specialized call to writeTestCaseResult
        if( rest[0] == "no actual" ){
            for ( tc=0; tc < testcases.length; tc++ ) {
                testcases[tc].passed = writeTestCaseResult(
                                    testcases[tc].expect,
                                    testcases[tc].actual,
                                    testcases[tc].description );
                testcases[tc].reason += checkReason(testcases[tc].passed);
        }
        }
    }
    stopTest();
    return ( testcases );
}


//  Compare expected result to the actual result and figure out whether
//  the test case passed.

function getTestCaseResult(expect,actual) {
    //  because ( NaN == NaN ) always returns false, need to do
    //  a special compare to see if we got the right result.
    if ( actual != actual ) {
        if ( typeof actual == "object" ) {
            actual = "NaN object";
        } else {
            actual = "NaN number";
        }
    }
    if ( expect != expect ) {
        if ( typeof expect == "object" ) {
            expect = "NaN object";
        } else {
            expect = "NaN number";
        }
    }
    var passed="";
    if (expect == actual) {
        if ( typeof(expect) != typeof(actual) ){
        passed = "type error";
        } else {
        passed = "true";
        }
    } else { //expect != actual
        passed = "false";
        // if both objects are numbers
        // need to replace w/ IEEE standard for rounding
        if (typeof(actual) == "number" && typeof(expect) == "number") {
        if ( Math.abs(actual-expect) < 0.0000001 ) {
            passed = "true";
        }
        }
    }
    return passed;
}

//  Begin printing functions.  These functions use the shell's
//  print function.  When running tests in the browser, these
//  functions, override these functions with functions that use
//  document.write.

function writeTestCaseResult( expect, actual, string ) {
    var passed = getTestCaseResult(expect,actual);
    var s = string;
    ___numtest++;
    if (passed == "true") {
        s = ". [" + s + "]";
    } else if (passed == "false") {
        s = "F [" + s + " expected: " + expect + "]";
	___failed++
    } else if (passed == "type error") {
        s = "F [" + s + " expected: " + expect + " and Type Mismatch: Expected Type: "+ typeof(expect) + ", Result Type: "+ typeof(actual) + "]";
	___failed++
    } else { //should never happen
        s = "F [" + s + " UNEXPECTED ERROR - see shell.as:writeTestCaseResult()]"
	___failed++
    }

    writeLineToLog(s);
    return passed;
}


function addToATS(expected, actual, description) {
    // Testcase Description
    this[fileName+"Str"].push(description);

    // Testcase Actual Values returned
    this[fileName].push(actual);

    // Testcase Expected Values Return
    this[fileName+"Ans"].push(expected);
}


//redundant, but leaving in in case its used elsewhere
/*
function writeFormattedResult( expect, actual, string, passed ) {
        var s = string ;
        s += ( passed ) ? PASSED : FAILED + expect;
        writeLineToLog( s);
        return passed;
}
*/

function writeLineToLog( string ) {
    _print( string );
}
function writeHeaderToLog( string ) {
    _print( string );
}
// end of print functions

//  When running in the shell, run the garbage collector after the
//  test has completed.

function stopTest(){
    // We cannot use testcases.length, because some tests use writeTestCaseResult directly
    if(___failed)
        trace("=====================Failures ("+___failed+"/"+___numtest+")=====================");
    else
        trace("=====================No failures (" + ___numtest + ")=====================");

    fscommand("quit");
}


//      Date functions used by tests in Date suite

//  Originally, the test suite used a hard-coded value TZ_DIFF = -8.
//  But that was only valid for testers in the Pacific Standard Time Zone!
//  We calculate the proper number dynamically for any tester. We just
//  have to be careful not to use a date subject to Daylight Savings Time...

function getTimeZoneDiff()
{
  return -((new Date(2000, 1, 1)).getTimezoneOffset())/60;
}

var msPerDay =          86400000;
var HoursPerDay =       24;
var MinutesPerHour =    60;
var SecondsPerMinute =  60;
var msPerSecond =       1000;
var msPerMinute =       60000;      //  msPerSecond * SecondsPerMinute
var msPerHour =         3600000;    //  msPerMinute * MinutesPerHour
var TZ_DIFF = getTimeZoneDiff();  // offset of tester's timezone from UTC
var             TZ_PST = -8;  // offset of Pacific Standard Time from UTC
var             TZ_IST = +5.5; // offset of Indian Standard Time from UTC
var             IST_DIFF = TZ_DIFF - TZ_IST;  // offset of tester's timezone from IST
var             PST_DIFF = TZ_DIFF - TZ_PST;  // offset of tester's timezone from PST
var TIME_1970    = 0;
var TIME_2000    = 946684800000;
var TIME_1900    = -2208988800000;
var now = new Date();
var TZ_DIFF = getTimeZoneDiff();
var TZ_ADJUST = TZ_DIFF * msPerHour;
var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
                     TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
var TIME_NOW = now.valueOf();





//  Date test "ResultArrays" are hard-coded for Pacific Standard Time.
//  We must adjust them for the tester's own timezone -

function adjustResultArray(ResultArray, msMode)
{
  // If the tester's system clock is in PST, no need to continue -
  if (!PST_DIFF) {return;}

  // The date testcases instantiate Date objects in two different ways:
  //
  //         millisecond mode: e.g.   dt = new Date(10000000);
  //         year-month-day mode:  dt = new Date(2000, 5, 1, ...);
  //
  //  In the first case, the date is measured from Time 0 in Greenwich (i.e. UTC).
  //  In the second case, it is measured with reference to the tester's local timezone.
  //
  //  In the first case we must correct those values expected for local measurements,
  //  like dt.getHours() etc. No correction is necessary for dt.getUTCHours() etc.
  //
  //  In the second case, it is exactly the other way around -

  var t;
  if (msMode)
  {
    // The hard-coded UTC milliseconds from Time 0 derives from a UTC date.
    // Shift to the right by the offset between UTC and the tester.
    t = ResultArray[TIME]  +  TZ_DIFF*msPerHour;

    // Use our date arithmetic functions to determine the local hour, day, etc.
    ResultArray[MINUTES] = MinFromTime(t);
    ResultArray[HOURS] = HourFromTime(t);
    ResultArray[DAY] = WeekDay(t);
    ResultArray[DATE] = DateFromTime(t);
    ResultArray[MONTH] = MonthFromTime(t);
    ResultArray[YEAR] = YearFromTime(t);
  }
  else
  {
    // The hard-coded UTC milliseconds from Time 0 derives from a PST date.
    // Shift to the left by the offset between PST and the tester.
    t = ResultArray[TIME]  -  PST_DIFF*msPerHour;

    // Use our date arithmetic functions to determine the UTC hour, day, etc.
    ResultArray[TIME] = t;
    ResultArray[UTC_MINUTES] = MinFromTime(t);
    ResultArray[UTC_HOURS] = HourFromTime(t);
    ResultArray[UTC_DAY] = WeekDay(t);
    ResultArray[UTC_DATE] = DateFromTime(t);
    ResultArray[UTC_MONTH] = MonthFromTime(t);
    ResultArray[UTC_YEAR] = YearFromTime(t);
  }
}


function Day( t ) {
    return ( Math.floor(t/msPerDay ) );
}
function DaysInYear( y ) {
    if ( y % 4 != 0 ) {
        return 365;
    }
    if ( (y % 4 == 0) && (y % 100 != 0) ) {
        return 366;
    }
    if ( (y % 100 == 0) &&  (y % 400 != 0) ) {
        return 365;
    }
    if ( (y % 400 == 0) ){
        return 366;
    } else {
        _print("ERROR: DaysInYear(" + y + ") case not covered");
        return Math.NaN; //"ERROR: DaysInYear(" + y + ") case not covered";
    }
}
function TimeInYear( y ) {
    return ( DaysInYear(y) * msPerDay );
}
function DayNumber( t ) {
    return ( Math.floor( t / msPerDay ) );
}
function TimeWithinDay( t ) {
    if ( t < 0 ) {
        return ( (t % msPerDay) + msPerDay );
    } else {
        return ( t % msPerDay );
    }
}
function YearNumber( t ) {
}
function TimeFromYear( y ) {
    return ( msPerDay * DayFromYear(y) );
}
function DayFromYear( y ) {
    return (    365*(y-1970) +
                Math.floor((y-1969)/4) -
                Math.floor((y-1901)/100) +
                Math.floor((y-1601)/400) );
}
function InLeapYear( t ) {
    if ( DaysInYear(YearFromTime(t)) == 365 ) {
        return 0;
    }
    if ( DaysInYear(YearFromTime(t)) == 366 ) {
        return 1;
    } else {
        return "ERROR:  InLeapYear("+ t + ") case not covered";
    }
}
function YearFromTime( t ) {
    t = Number( t );
    var sign = ( t < 0 ) ? -1 : 1;
    var year = ( sign < 0 ) ? 1969 : 1970;

    for (   var timeToTimeZero = t; ;  ) {
    //  subtract the current year's time from the time that's left.
        timeToTimeZero -= sign * TimeInYear(year)
        if (isNaN(timeToTimeZero))
            return NaN;

    //  if there's less than the current year's worth of time left, then break.
        if ( sign < 0 ) {
            if ( sign * timeToTimeZero <= 0 ) {
                break;
            } else {
                year += sign;
            }
        } else {
            if ( sign * timeToTimeZero < 0 ) {
                break;
            } else {
                year += sign;
            }
        }
    }
    return ( year );
}
function MonthFromTime( t ) {
    //  i know i could use switch but i'd rather not until it's part of ECMA
    var day = DayWithinYear( t );
    var leap = InLeapYear(t);

    if ( (0 <= day) && (day < 31) ) {
        return 0;
    }
    if ( (31 <= day) && (day < (59+leap)) ) {
        return 1;
    }
    if ( ((59+leap) <= day) && (day < (90+leap)) ) {
        return 2;
    }
    if ( ((90+leap) <= day) && (day < (120+leap)) ) {
        return 3;
    }
    if ( ((120+leap) <= day) && (day < (151+leap)) ) {
        return 4;
    }
    if ( ((151+leap) <= day) && (day < (181+leap)) ) {
        return 5;
    }
    if ( ((181+leap) <= day) && (day < (212+leap)) ) {
        return 6;
    }
    if ( ((212+leap) <= day) && (day < (243+leap)) ) {
        return 7;
    }
    if ( ((243+leap) <= day) && (day < (273+leap)) ) {
        return 8;
    }
    if ( ((273+leap) <= day) && (day < (304+leap)) ) {
        return 9;
    }
    if ( ((304+leap) <= day) && (day < (334+leap)) ) {
        return 10;
    }
    if ( ((334+leap) <= day) && (day < (365+leap)) ) {
        return 11;
    } else {
        return "ERROR:  MonthFromTime("+t+") not known";
    }
}
function DayWithinYear( t ) {
        return( Day(t) - DayFromYear(YearFromTime(t)));
}
function DateFromTime( t ) {
    var day = DayWithinYear(t);
    var month = MonthFromTime(t);
    if ( month == 0 ) {
        return ( day + 1 );
    }
    if ( month == 1 ) {
        return ( day - 30 );
    }
    if ( month == 2 ) {
        return ( day - 58 - InLeapYear(t) );
    }
    if ( month == 3 ) {
        return ( day - 89 - InLeapYear(t));
    }
    if ( month == 4 ) {
        return ( day - 119 - InLeapYear(t));
    }
    if ( month == 5 ) {
        return ( day - 150- InLeapYear(t));
    }
    if ( month == 6 ) {
        return ( day - 180- InLeapYear(t));
    }
    if ( month == 7 ) {
        return ( day - 211- InLeapYear(t));
    }
    if ( month == 8 ) {
        return ( day - 242- InLeapYear(t));
    }
    if ( month == 9 ) {
        return ( day - 272- InLeapYear(t));
    }
    if ( month == 10 ) {
        return ( day - 303- InLeapYear(t));
    }
    if ( month == 11 ) {
        return ( day - 333- InLeapYear(t));
    }

    return ("ERROR:  DateFromTime("+t+") not known" );
}
function WeekDay( t ) {
    var weekday = (Day(t)+4) % 7;
    return( weekday < 0 ? 7 + weekday : weekday );
}

// missing daylight savins time adjustment

function HourFromTime( t ) {
    var h = Math.floor( t / msPerHour ) % HoursPerDay;
    return ( (h<0) ? HoursPerDay + h : h  );
}
function MinFromTime( t ) {
    var min = Math.floor( t / msPerMinute ) % MinutesPerHour;
    return( ( min < 0 ) ? MinutesPerHour + min : min  );
}
function SecFromTime( t ) {
    var sec = Math.floor( t / msPerSecond ) % SecondsPerMinute;
    return ( (sec < 0 ) ? SecondsPerMinute + sec : sec );
}
function msFromTime( t ) {
    var ms = t % msPerSecond;
    return ( (ms < 0 ) ? msPerSecond + ms : ms );
}
function LocalTZA() {
    return ( TZ_DIFF * msPerHour );
}
function UTC( t ) {
    return ( t - LocalTZA() - DaylightSavingTA(t - LocalTZA()) );
}

function DaylightSavingTA( t ) {
    // There is no Daylight saving time in India
    if (IST_DIFF == 0)
        return 0;

    var dst_start;
    var dst_end;
    // Windows fix for 2007 DST change made all previous years follow new DST rules
    // create a date pre-2007 when DST is enabled according to 2007 rules
    var pre_2007:Date = new Date("Mar 20 2006");
    // create a date post-2007
    var post_2007:Date = new Date("Mar 20 2008");
    // if the two dates timezoneoffset match, then this must be a windows box applying
    // post-2007 DST rules to earlier dates.
    var win_machine:Boolean = pre_2007.timezoneOffset == post_2007.timezoneOffset

    if (TZ_DIFF<=-4 && TZ_DIFF>=-8) {
        if (win_machine || YearFromTime(t)>=2007) {
            dst_start = GetSecondSundayInMarch(t) + 2*msPerHour;
            dst_end = GetFirstSundayInNovember(t) + 2*msPerHour;
        } else {
            dst_start = GetFirstSundayInApril(t) + 2*msPerHour;
            dst_end = GetLastSundayInOctober(t) + 2*msPerHour;
        }
    } else {
        dst_start = GetLastSundayInMarch(t) + 2*msPerHour;
        dst_end = GetLastSundayInOctober(t) + 2*msPerHour;
    }
    if ( t >= dst_start && t < dst_end ) {
        return msPerHour;
    } else {
        return 0;
                
    }

    // Daylight Savings Time starts on the second Sunday    in March at 2:00AM in
    // PST.  Other time zones will need to override this function.
    _print( new Date( UTC(dst_start + LocalTZA())) );

    return UTC(dst_start  + LocalTZA());
}
function GetLastSundayInMarch(t) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);
    var march = TimeFromYear(year) + TimeInMonth(0,leap) +  TimeInMonth(1,leap)-LocalTZA()+2*msPerHour;
    var sunday;
    for( sunday=march;WeekDay(sunday)>0;sunday +=msPerDay ){;}
    var last_sunday;
    while (true) {
       sunday=sunday+7*msPerDay;
       if (MonthFromTime(sunday)>2)
           break;
       last_sunday=sunday;
    }
    return last_sunday;
}
function GetSecondSundayInMarch(t ) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);
    var march = TimeFromYear(year) + TimeInMonth(0,leap) +  TimeInMonth(1,leap)-LocalTZA()+2*msPerHour;
        
    for ( var first_sunday = march; WeekDay(first_sunday) >0;
        first_sunday +=msPerDay )
    {
        ;
    }
    second_sunday=first_sunday+7*msPerDay;
    return second_sunday;
}



function GetFirstSundayInNovember( t ) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);
        var     nov,m;
    for ( nov = TimeFromYear(year), m = 0; m < 10; m++ )    {
        nov += TimeInMonth(m, leap);
    }
    nov=nov-LocalTZA()+2*msPerHour;
    for ( var first_sunday =    nov; WeekDay(first_sunday)  > 0;
        first_sunday    += msPerDay )
    {
        ;
    }
    return first_sunday;
}
function GetFirstSundayInApril( t ) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);
    var     apr,m;
    for ( apr = TimeFromYear(year), m = 0; m < 3; m++ ) {
        apr += TimeInMonth(m, leap);
    }
    apr=apr-LocalTZA()+2*msPerHour;

    for ( var first_sunday =    apr; WeekDay(first_sunday)  > 0;
        first_sunday    += msPerDay )
    {
        ;
    }
    return first_sunday;
}
function GetLastSundayInOctober(t) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);
    var oct,m;
    for (oct =  TimeFromYear(year), m = 0; m < 9; m++ ) {
        oct += TimeInMonth(m, leap);
    }
    oct=oct-LocalTZA()+2*msPerHour;
    var sunday;
    for( sunday=oct;WeekDay(sunday)>0;sunday +=msPerDay ){;}
    var last_sunday;
    while (true) {
       last_sunday=sunday;
       sunday=sunday+7*msPerDay;
       if (MonthFromTime(sunday)>9)
           break;
    }
    return last_sunday;
}
function LocalTime( t ) {
    return ( t + LocalTZA() + DaylightSavingTA(t) );
}
function MakeTime( hour, min, sec, ms ) {
    if ( isNaN( hour ) || isNaN( min ) || isNaN( sec ) || isNaN( ms ) ) {
        return Number.NaN;
    }

    hour = ToInteger(hour);
    min  = ToInteger( min);
    sec  = ToInteger( sec);
    ms   = ToInteger( ms );

    return( (hour*msPerHour) + (min*msPerMinute) +
            (sec*msPerSecond) + ms );
}
function MakeDay( year, month, date ) {
    if ( isNaN(year) || isNaN(month) || isNaN(date) ) {
        return Number.NaN;
    }
    year = ToInteger(year);
    month = ToInteger(month);
    date = ToInteger(date );

    var sign = ( year < 1970 ) ? -1 : 1;
    var t =    ( year < 1970 ) ? 1 :  0;
    var y =    ( year < 1970 ) ? 1969 : 1970;

    var result5 = year + Math.floor( month/12 );
    var result6 = month % 12;

    if ( year < 1970 ) {
       for ( y = 1969; y >= year; y += sign ) {
         t += sign * TimeInYear(y);
       }
    } else {
        for ( y = 1970 ; y < year; y += sign ) {
            t += sign * TimeInYear(y);
        }
    }

    var leap = InLeapYear( t );

    for ( var m = 0; m < month; m++ ) {
        t += TimeInMonth( m, leap );
    }

    if ( YearFromTime(t) != result5 ) {
        return Number.NaN;
    }
    if ( MonthFromTime(t) != result6 ) {
        return Number.NaN;
    }
    if ( DateFromTime(t) != 1 ) {
        return Number.NaN;
    }

    return ( (Day(t)) + date - 1 );
}
function TimeInMonth( month, leap ) {
    // september april june november
    // jan 0  feb 1  mar 2  apr 3   may 4  june 5  jul 6
    // aug 7  sep 8  oct 9  nov 10  dec 11

    if ( month == 3 || month == 5 || month == 8 || month == 10 ) {
        return ( 30*msPerDay );
    }

    // all the rest
    if ( month == 0 || month == 2 || month == 4 || month == 6 ||
         month == 7 || month == 9 || month == 11 ) {
        return ( 31*msPerDay );
     }

    // save february
    return ( (leap == 0) ? 28*msPerDay : 29*msPerDay );
}
function MakeDate( day, time ) {
    if (    day == Number.POSITIVE_INFINITY ||
            day == Number.NEGATIVE_INFINITY ||
            day == Number.NaN ) {
        return Number.NaN;
    }
    if (    time == Number.POSITIVE_INFINITY ||
            time == Number.POSITIVE_INFINITY ||
            day == Number.NaN) {
        return Number.NaN;
    }
    return ( day * msPerDay ) + time;
}


// Compare 2 dates, they are considered equal if the difference is less than 1 second
function compareDate(d1, d2) {
    //Dates may be off by a second
    if (d1 == d2) {
        return true;
    } else if (Math.abs(new Date(d1) - new Date(d2)) <= 1000) {
        return true;
    } else {
        return false;
    }
}

function TimeClip( t ) {
    if ( isNaN( t ) ) {
        return ( Number.NaN );
    }
    if ( Math.abs( t ) > 8.64e15 ) {
        return ( Number.NaN );
    }

    return ( ToInteger( t ) );
}
function ToInteger( t ) {
    t = Number( t );

    if ( isNaN( t ) ){
        return ( Number.NaN );
    }
    if ( t == 0 || t == -0 ||
         t == Number.POSITIVE_INFINITY || t == Number.NEGATIVE_INFINITY ) {
         return 0;
    }

    var sign = ( t < 0 ) ? -1 : 1;

    return ( sign * Math.floor( Math.abs( t ) ) );
}
function Enumerate ( o ) {
    var p;
    for ( p in o ) {
        _print( p +": " + o[p] );
    }
}



function START(summary)
{
      // print out bugnumber

     /*if ( BUGNUMBER ) {
              writeLineToLog ("BUGNUMBER: " + BUGNUMBER );
      }*/
    XML.setSettings (null);
    testcases = new Array();

    // text field for results
    tc = 0;
    /*this.addChild ( tf );
    tf.x = 30;
    tf.y = 50;
    tf.width = 200;
    tf.height = 400;*/

    _print(summary);
    var summaryParts = summary.split(" ");
    _print("section: " + summaryParts[0] + "!");
    //fileName = summaryParts[0];

}

function BUG(bug)
{
  printBugNumber(bug);
}

function reportFailure (section, msg)
{
    print(FAILED + inSection(section)+"\n"+msg);
    /*var lines = msg.split ("\n");
    for (var i=0; i<lines.length; i++)
        print(FAILED + lines[i]);
    */
}

function TEST(section, expected, actual)
{
    AddTestCase(section, expected, actual);
}

function myGetNamespace (obj, ns) {
    if (ns != undefined) {
        return obj.namespace(ns);
    } else {
        return obj.namespace();
    }
}

function TEST_XML(section, expected, actual)
{
  var actual_t = typeof actual;
  var expected_t = typeof expected;

  if (actual_t != "xml") {
    // force error on type mismatch
    TEST(section, new XML(), actual);
    return;
  }

  if (expected_t == "string") {

    TEST(section, expected, actual.toXMLString());
  } else if (expected_t == "number") {

    TEST(section, String(expected), actual.toXMLString());
  } else {
    reportFailure ("Bad TEST_XML usage: type of expected is "+expected_t+", should be number or string");
  }
}

function SHOULD_THROW(section)
{
  reportFailure(section, "Expected to generate exception, actual behavior: no exception was thrown");
}

function END()
{
    test();
}

function NL()
{
  //return java.lang.System.getProperty("line.separator");
  return "\n";
}

function printBugNumber (num)
{
  //writeLineToLog (BUGNUMBER + num);
}

function toPrinted(value)
{
  if (typeof value == "xml") {
    return value.toXMLString();
  } else {
    return String(value);
  }
}

function grabError(err, str) {
    var typeIndex = str.indexOf("Error:");
    var type = str.substr(0, typeIndex + 5);
    if (type == "TypeError") {
        AddTestCase("Asserting for TypeError", true, (err is TypeError));
    } else if (type == "ArgumentError") {
        AddTestCase("Asserting for ArgumentError", true, (err is ArgumentError));
    }
    var numIndex = str.indexOf("Error #");
    var num;
    if (numIndex >= 0) {
        num = str.substr(numIndex, 11);
    } else {
        num = str;
    }
    return num;
}

function AddErrorTest(desc:String, expectedErr:String, testFunc:Function) {
    actualErr = null;
    try {
        testFunc();
    } catch (e) {
        actualErr = e;
    }
    grabError(actualErr, expectedErr);
    AddTestCase(desc, expectedErr, actualErr.toString().substr(0, expectedErr.length));
}

var cnNoObject = 'Unexpected Error!!! Parameter to this function must be an object';
var cnNoClass = 'Unexpected Error!!! Cannot find Class property';
var cnObjectToString = Object.prototype.toString;

// checks that it's safe to call findType()
function getJSType(obj)
{
  if (isObject(obj))
    return findType(obj);
  return cnNoObject;
}


// checks that it's safe to call findType()
function getJSClass(obj)
{
  if (isObject(obj))
    return findClass(findType(obj));
  return cnNoObject;
}
function isObject(obj)
{
  return obj instanceof Object;
}

function findType(obj)
{
  return cnObjectToString.apply(obj);
}
// given '[object Number]',  return 'Number'
function findClass(sType)
{
  var re =  /^\[.*\s+(\w+)\s*\]$/;
  var a = sType.match(re);

  if (a && a[1])
    return a[1];
  return cnNoClass;
}
function inSection(x) {
   return "Section "+x+" of test -";
}
function printStatus (msg)
{
    var lines = msg.split ("\n");
    var l;

    for (var i=0; i<lines.length; i++)
        _print(STATUS + lines[i]);

}
function reportCompare (expected, actual, description)
{
    AddTestCase(description, expected, actual);
    /*
    var expected_t = typeof expected;
    var actual_t = typeof actual;
    var output = "";
    if ((VERBOSE) && (typeof description != "undefined"))
            printStatus ("Comparing '" + description + "'");

    if (expected_t != actual_t)
            output += "Type mismatch, expected type " + expected_t +
                ", actual type " + actual_t + "\n";
    else if (VERBOSE)
            printStatus ("Expected type '" + actual_t + "' matched actual " +
                         "type '" + expected_t + "'");

    if (expected != actual)
            output += "Expected value '" + expected + "', Actual value '" + actual +
                "'\n";
    else if (VERBOSE)
            printStatus ("Expected value '" + actual + "' matched actual " +
                         "value '" + expected + "'");

    if (output != "")
        {
            if (typeof description != "undefined")
                reportFailure (description);
                reportFailure (output);
        }
    stopTest();
    */
}
// encapsulate output in shell
function _print(s) {
  trace(s);
//  trace(s);
}

// workaround for Debugger vm where error contains more details
function parseError(error,len) {
  if (error.length>len) {
    error=error.substring(0,len);
  }
  return error;
}

// helper function for api versioning tests
function versionTest(testFunc, desc, expected) {
   var result;
   try {
       result = testFunc();
   } catch (e) {
       // Get the error type and code, but not desc if its a debug build
       result = e.toString().substring(0,e.toString().indexOf(':')+13);
   }
   AddTestCase(desc, expected, result);
}

// Helper functions for tests from spidermonkey (JS1_5 and greater)
/*
  Calculate the "order" of a set of data points {X: [], Y: []}
  by computing successive "derivatives" of the data until
  the data is exhausted or the derivative is linear.
*/
function BigO(data) {
    var order = 0;
    var origLength = data.X.length;

    while (data.X.length > 2) {
        var lr = new LinearRegression(data);
        if (lr.b > 1e-6) {
            // only increase the order if the slope
            // is "great" enough
            order++;
        }

        if (lr.r > 0.98 || lr.Syx < 1 || lr.b < 1e-6) {
            // terminate if close to a line lr.r
            // small error lr.Syx
            // small slope lr.b
            break;
        }
        data = dataDeriv(data);
    }

    if (2 == origLength - order) {
        order = Number.POSITIVE_INFINITY;
    }
    return order;

    function LinearRegression(data) {
        /*
      y = a + bx
      for data points (Xi, Yi); 0 <= i < n

      b = (n*SUM(XiYi) - SUM(Xi)*SUM(Yi))/(n*SUM(Xi*Xi) - SUM(Xi)*SUM(Xi))
      a = (SUM(Yi) - b*SUM(Xi))/n
    */
        var i;

        if (data.X.length != data.Y.length) {
            throw 'LinearRegression: data point length mismatch';
        }
        if (data.X.length < 3) {
            throw 'LinearRegression: data point length < 2';
        }
        var n = data.X.length;
        var X = data.X;
        var Y = data.Y;

        this.Xavg = 0;
        this.Yavg = 0;

        var SUM_X = 0;
        var SUM_XY = 0;
        var SUM_XX = 0;
        var SUM_Y = 0;
        var SUM_YY = 0;

        for (i = 0; i < n; i++) {
            SUM_X += X[i];
            SUM_XY += X[i] * Y[i];
            SUM_XX += X[i] * X[i];
            SUM_Y += Y[i];
            SUM_YY += Y[i] * Y[i];
        }

        this.b = (n * SUM_XY - SUM_X * SUM_Y) / (n * SUM_XX - SUM_X * SUM_X);
        this.a = (SUM_Y - this.b * SUM_X) / n;

        this.Xavg = SUM_X / n;
        this.Yavg = SUM_Y / n;

        var SUM_Ydiff2 = 0;
        var SUM_Xdiff2 = 0;
        var SUM_XdiffYdiff = 0;

        for (i = 0; i < n; i++) {
            var Ydiff = Y[i] - this.Yavg;
            var Xdiff = X[i] - this.Xavg;

            SUM_Ydiff2 += Ydiff * Ydiff;
            SUM_Xdiff2 += Xdiff * Xdiff;
            SUM_XdiffYdiff += Xdiff * Ydiff;
        }

        var Syx2 = (SUM_Ydiff2 - Math.pow(SUM_XdiffYdiff / SUM_Xdiff2, 2)) / (n - 2);
        var r2 = Math.pow((n * SUM_XY - SUM_X * SUM_Y), 2) / ((n * SUM_XX - SUM_X * SUM_X) * (n * SUM_YY - SUM_Y * SUM_Y));

        this.Syx = Math.sqrt(Syx2);
        this.r = Math.sqrt(r2);

    }

    function dataDeriv(data) {
        if (data.X.length != data.Y.length) {
            throw 'length mismatch';
        }
        var length = data.X.length;

        if (length < 2) {
            throw 'length ' + length + ' must be >= 2';
        }
        var X = data.X;
        var Y = data.Y;

        var deriv = {
            X: [],
            Y: []
        };

        for (var i = 0; i < length - 1; i++) {
            deriv.X[i] = (X[i] + X[i + 1]) / 2;
            deriv.Y[i] = (Y[i + 1] - Y[i]) / (X[i + 1] - X[i]);
        }
        return deriv;
    }

    return 0;
}

/*
 * Date: 07 February 2001
 *
 * Functionality common to Array testing -
 */
//-----------------------------------------------------------------------------

var gTestsubsuite = 'Expressions';

var CHAR_LBRACKET = '[';
var CHAR_RBRACKET = ']';
var CHAR_QT_DBL = '"';
var CHAR_QT = "'";
var CHAR_NL = '\n';
var CHAR_COMMA = ',';
var CHAR_SPACE = ' ';
var TYPE_STRING = typeof 'abc';


/*
 * If available, arr.toSource() gives more detail than arr.toString()
 *
 * var arr = Array(1,2,'3');
 *
 * arr.toSource()
 * [1, 2, "3"]
 *
 * arr.toString()
 * 1,2,3
 *
 * But toSource() doesn't exist in Rhino, so use our own imitation, below -
 *
 */
function formatArray(arr)
{
  try
  {
    return arr.toSource();
  }
  catch(e)
  {
    return toSource(arr);
  }
}



/*
 * Imitate SpiderMonkey's arr.toSource() method:
 *
 * a) Double-quote each array element that is of string type
 * b) Represent |undefined| and |null| by empty strings
 * c) Delimit elements by a comma + single space
 * d) Do not add delimiter at the end UNLESS the last element is |undefined|
 * e) Add square brackets to the beginning and end of the string
 */
function toSource(arr)
{
  var delim = CHAR_COMMA + CHAR_SPACE;
  var elt = '';
  var ret = '';
  var len = arr.length;

  for (i=0; i<len; i++)
  {
    elt = arr[i];

    switch(true)
    {
    case (typeof elt === TYPE_STRING) :
      ret += doubleQuote(elt);
      break;

    case (elt === undefined || elt === null) :
      break; // add nothing but the delimiter, below -

    default:
      ret += elt.toString();
    }

    if ((i < len-1) || (elt === undefined))
      ret += delim;
  }

  return  CHAR_LBRACKET + ret + CHAR_RBRACKET;
}


function doubleQuote(text)
{
  return CHAR_QT_DBL + text + CHAR_QT_DBL;
}


function singleQuote(text)
{
  return CHAR_QT + text + CHAR_QT;
}

function removeExceptionDetail(s:String) {
    var fnd=s.indexOf(" ");
    if (fnd>-1) {
        if (s.indexOf(':',fnd)>-1) {
                s=s.substring(0,s.indexOf(':',fnd));
        }
    }
    return s;
}

function sortObject(o:Object) {
    var keys=[];
    for ( key in o ) {
        if (o==undefined) {
           continue;
        }
        keys[keys.length]=key;
    }
    keys.sort();
    var ret="{";
    for (i in keys) {
        key=keys[i];
        value=o[key];
        if (value is String) {
            value='"'+value+'"';
        } else if (value is Array) {
            value='['+value+']';
        } else if (value is Object) {
        }
        ret += '"'+key+'":'+value+",";
    }
    ret=ret.substring(0,ret.length-1);
    ret+="}";
    return ret;
}
