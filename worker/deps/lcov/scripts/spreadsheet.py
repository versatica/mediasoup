#!/usr/bin/env python3

import xlsxwriter
import argparse
import json
import pdb
import datetime
import os.path
import os
import sys

from xlsxwriter.utility import xl_rowcol_to_cell
from functools import cmp_to_key

devMinThreshold = 1.5
devMaxThreshold = 2.0
thresholdPercent = 0.15

class GenerateSpreadsheet(object):

    def __init__(self, excelFile, files, args):

        s = xlsxwriter.Workbook(excelFile)

        # keep a list of sheets so we can insert a summary..
        geninfoSheets = []
        summarySheet = s.add_worksheet("geninfo_summary") if 1 < len(files) else None

        # order:  order of processing
        # file: time to process one GCDA file
        # parse:  time to generate and read gcov data
        # exec: time to execute gcov
        # append: to merge file info into parent
        geninfoKeys = ('order', 'file', 'parse', 'exec', 'append')

        # work: productive time: process_one_chunk + merge chunk
        # chunk: everything from fork() to end of filesytem cleanup after child merge
        # child: time from entering child process to immediately before serialize
        # process: time to call process_one_chunk
        # undump:  time to deserialize chunk data into master
        # queue: time between child finish and start of merge in parent
        # merge: time to merge returned chunk info
        geninfoChunkKeys = ('work', 'chunk', 'queue', 'child', 'process', 'undump', 'merge')
        geninfoSpecialKeys = ('total', 'parallel', 'filter', 'write')

        # keys related to filtering
        filterKeys = ('filt_chunk', 'filt_queue',  'filt_child', 'filt_proc', 'filt_undump', 'filt_merge')
        if args.verbose:
            geninfoKeys.extend(['read', 'translate'])

        self.formats = {
            'twoDecimal': s.add_format({'num_format': '0.00'}),
            'intFormat': s.add_format({'num_format': '0'}),
            'title': s.add_format({'bold': True,
                                   'align': 'center',
                                   'valign': 'vcenter',
                                   'text_wrap': True}),
            'italic': s.add_format({'italic': True,
                                    'align': 'center',
                                    'valign': 'vcenter'}),
            'highlight': s.add_format({'bg_color': 'yellow'}),
            'danger': s.add_format({'bg_color': 'red'}),
            'good': s.add_format({'bg_color': 'green'}),
        }
        intFormat = self.formats['intFormat']
        twoDecimal = self.formats['twoDecimal']

        def insertConditional(sheet, avgRow, devRow,
                              beginRow, beginCol, endRow, endCol):
            # absolute row, relative column
            avgCell = xl_rowcol_to_cell(avgRow, beginCol, True, False)
            devCell = xl_rowcol_to_cell(devRow, beginCol, True, False)
            # relative row, relative column
            dataCell = xl_rowcol_to_cell(beginRow, beginCol, False, False)
            # absolute value of differnce from the average
            diff = 'ABS(%(cell)s - %(avg)s)' % {
                'cell' : dataCell,
                'avg' : avgCell,
            }

            # min difference is difference > 15% of average
            #  only look at positive difference:  taking MORE than average time
            threshold = '(%(cell)s - %(avg)s) > (%(percent)f * %(avg)s)' % {
                'cell' : dataCell,
                'avg' : avgCell,
                'percent': thresholdPercent,
            }

            # cell not blank and difference > 2X std.dev and > 15% of average
            dev2 = '=AND(NOT(OR(ISBLANK(%(cell)s),ISBLANK(%(dev)s))), %(diff)s > (%(devMaxThresh)f * %(dev)s), %(threshold)s)' % {
                'diff' : diff,
                'threshold' : threshold,
                'cell' : dataCell,
                'avg' : avgCell,
                'dev' : devCell,
                'devMaxThresh': devMaxThreshold,
            }
            # yellow if between 1.5 and 2 standard deviations away
            dev1 = '=AND(NOT(OR(ISBLANK(%(cell)s),ISBLANK(%(dev)s))), %(diff)s >  (%(devMinThresh)f * %(dev)s), %(diff)s <= (%(devMaxThresh)f * %(dev)s), %(threshold)s) ' % {
                'diff' : diff,
                'threshold' : threshold,
                'cell' : dataCell,
                'avg' : avgCell,
                'dev' : devCell,
                'devMaxThresh': devMaxThreshold,
                'devMinThresh': devMinThreshold,
            }
            # yellow if between 1 and 2 standard deviations away
            sheet.conditional_format(beginRow, beginCol, endRow, endCol,
                                     { 'type': 'formula',
                                       'criteria': dev1,
                                       'format' : self.formats['highlight'],
                                   })
            # red if more than 2 2 standard deviations away
            sheet.conditional_format(beginRow, beginCol, endRow, endCol,
                                     { 'type': 'formula',
                                       'criteria': dev2,
                                       'format' : self.formats['danger'],
                                   })
            # green if more than 1.5 standard deviations better
            good = '=AND(NOT(OR(ISBLANK(%(cell)s),ISBLANK(%(dev)s))), (%(cell)s - %(avg)s) < (%(devMaxThresh)f * -%(dev)s), %(threshold)s)' % {
                'cell' : dataCell,
                'threshold' : threshold,
                'cell' : dataCell,
                'avg' : avgCell,
                'dev' : devCell,
                'devMaxThresh': devMaxThreshold,
            }
            sheet.conditional_format(beginRow, beginCol, endRow, endCol,
                                     { 'type': 'formula',
                                       'criteria': good,
                                       'format' : self.formats['good'],
                                   })

        def insertStats(keys, sawData, sumRow, avgRow, devRow, beginRow, endRow, col):
            firstCol = col
            col -= 1
            for key in keys:
                col += 1
                if key in ('order',):
                    continue
                f = xl_rowcol_to_cell(beginRow, col)
                t = xl_rowcol_to_cell(endRow, col)

                if key not in sawData:
                    continue
                sum = "+SUM(%(from)s:%(to)s)" % {
                    "from" : f,
                    "to": t
                }
                sheet.write_formula(sumRow, col, sum, twoDecimal)
                avg = "+AVERAGE(%(from)s:%(to)s)" % {
                    'from': f,
                    'to': t,
                }
                sheet.write_formula(avgRow, col, avg, twoDecimal)
                if sawData[key] < 2:
                    continue
                dev = "+STDEV(%(from)s:%(to)s)" % {
                    'from': f,
                    'to': t,
                }
                sheet.write_formula(devRow, col, dev, twoDecimal)

            insertConditional(sheet, avgRow, devRow,
                              beginRow, firstCol, endRow, col)

        for name in files:
            try:
                with open(name) as f:
                    data = json.load(f)
            except Exception as err:
                print("%s: unable to parse: %s" % (name, str(err)))
                continue

            try:
                tool = data['config']['tool']
            except:
                tool = 'unknown'
                print("%s: unknown tool" %(name))

            p, f = os.path.split(name)
            if os.path.splitext(f)[0] == tool:
                sheetname = os.path.split(p)[1] # the directory
            else:
                sheetname = f
            if len(sheetname) > 30:
                # take the tail of the string..
                sheetname = sheetname[-30:]
            sn = sheetname
            for i in range(1000):
                try:
                    sheet = s.add_worksheet(sn[-31:])
                    break
                except:
                    sn = sheetname + "_" + str(i)
            else:
                print("%s in use..giving up" % (sheetname))
                sys.exit(1)

            try:
                parallel = data['config']['maxParallel']
            except:
                parallel = 0

            row = 0
            sheet.write_string(row, 0, name)
            row += 1
            sheet.write_string(row, 0, 'config')
            for n in sorted(data['config'].keys()):
                try:
                    sheet.write_string(row, 1, n)
                    if n in ("tool", 'date', ):
                        sheet.write_string(row, 2, data['config'][n])
                    else:
                        sheet.write_number(row, 2, data['config'][n], intFormat)
                    row += 1
                except:
                    # old file format..skip it
                    pass

            if tool == 'geninfo':
                for k in ('chunkSize', 'nChunks', 'nFiles', 'interval'):
                    try:
                        sheet.write_number(row, 2, data[k], intFormat);
                        sheet.write_string(row, 1, k)
                        row += 1
                    except:
                        pass

            for k in ('total', 'overall'):
                if k in data:
                    sheet.write_string(row, 0, 'total')
                    sheet.write_number(row, 1, data[k], twoDecimal)
                    total = xl_rowcol_to_cell(row, 1)
                    totalRow = row
                    row += 1

            if tool == 'lcov':
                # is this a parallel execution?
                try:
                    segments = data['config']['segments']

                    effectiveParallelism = ""
                    sep = "+("
                    for seg in range(segments):
                        sheet.write_string(row, 0, 'segment %d' % (seg))
                        try:
                            d = data[seg]
                        except:
                            d = data[str(seg)]

                        start = row
                        for k in ('total', 'merge', 'undump'):
                            sheet.write_string(row, 1, k)
                            try:
                                sheet.write_number(row, 2, float(d[k]), twoDecimal)
                                if k == 'total':
                                    effectiveParallelism += sep + xl_rowcol_to_cell(row, 2)
                                    sep = "+"
                            except:
                                print("%s: failed to write %s for lcov[seg %d][%s]" % (
                                    name, str(d[k]) if k in d else "??", seg, k))
                            row += 1
                        begin = row
                        for k in ('parse', 'append'):
                            try:
                                # don't crash on partially corrupt profile data
                                d2 = d[k]
                                sheet.write_string(row, 1, k)
                                for f in sorted(d2.keys()):
                                    sheet.write_string(row, 2, f)
                                    try:
                                        sheet.write_number(row, 3, float(d2[f]), twoDecimal)
                                    except:
                                        print("%s: failed to write %s for lcov[seg %d][%s][$s]" % (name, str(d2[f]), seg, k, f))
                                row += 1
                            except:
                                print("%s: failed to write %s for lcov[seg %d]" % (name, k, seg))
                    effectiveParallelism += ")/%(total)s" % {
                        'total': total,
                    }
                    sheet.write_formula(totalRow, 3, effectiveParallelism, twoDecimal)


                except Exception as err:

                    # not segmented - just print everything...
                    for k in ('total', 'merge', 'undump'):
                        sheet.write_string(row, 1, k)
                        val = 'NA'
                        try:
                            val = data[k]
                            sheet.write_number(row, 2, float(val), twoDecimal)
                        except:
                            print("%s: failed to write %s for lcov[%s]" % (name, str(val), k))
                            row += 1
                    for k in ('parse', 'append'):
                        try:
                            d2 = data[k]
                            sheet.write_string(row, 1, k)
                            for f in sorted(d2.keys()):
                                sheet.write_string(row, 2, f)
                                try:
                                    sheet.write_number(row, 3, float(d2[f]), twoDecimal)
                                except:
                                    print("%s: failed to write %s for lcov[%s][$s]" % (name, str(d2[f]), k, f))
                            row += 1
                        except:
                            print("%s: failed to find key '%s'" %(name, k))

                # go on to the next file
                continue

            elif tool == 'geninfo':

                summaryKeys = (*geninfoSpecialKeys, *geninfoChunkKeys, *geninfoKeys)
                if args.show_filter:
                    summaryKeys = (*geninfoSpecialKeys, *geninfoChunkKeys, *geninfoKeys, *filterKeys)
                if summarySheet:
                    # first one - add titles, etc
                    title = self.formats['title']

                    if len(geninfoSheets) == 0:
                        summarySheet.write_string(1, 0, "average", title)
                        summarySheet.write_string(2, 0, "stddev", title)
                        titleRow = 0
                        summarySheet.write_string(titleRow, 0, "case", title)
                        col = 1
                        for k in summaryKeys:
                            if k in ('order',):
                                continue
                            summarySheet.write_string(titleRow, col, k, title)
                            col += 1
                            if k in geninfoSpecialKeys:
                                continue
                            summarySheet.write_string(titleRow, col, k + ' avg', title)
                            col += 1
                        summarySheet.write_string(3, 0, "YELLOW: Value between [%(devMinThresh)0.2f,%(devMaxThresh)0.2f) standard deviations larger than average" % {
                            'devMinThresh': devMinThreshold,
                            'devMaxThresh': devMaxThreshold,
                        }, self.formats['highlight'])
                        summarySheet.write_string(4, 0, "RED: Value more than %(devMaxThresh)0.2f standard deviations larger than average" % {
                            'devMaxThresh': devMaxThreshold,
                        }, self.formats['danger'])
                        summarySheet.write_string(5, 0, "GREEN: Value more than %(devMaxThresh)0.2f standard deviations smaller than average" % {
                            'devMaxThresh': devMaxThreshold,
                        }, self.formats['good'])
                        firstSummaryRow = 7

                    # want rows for average and variance - leave a blank row
                    summaryRow = firstSummaryRow + len(geninfoSheets)

                geninfoSheets.append(sheet)
                # already inserted the ''total' entry
                specialsStart = row - 1
                for k in geninfoSpecialKeys[1:]:
                    try:
                        sheet.write_string(row, 0, k)
                        sheet.write_number(row, 1, data[k], twoDecimal)
                    except:
                        pass
                    row += 1;

                sawData = {}
                sawData['total'] = 0
                sheet.write_string(row, 0, 'find')
                row += 1
                for dirname in sorted(data['find'].keys()):
                    sheet.write_string(row, 1, dirname)
                    sheet.write_number(row, 2, data['find'][dirname], twoDecimal)
                    row += 1

                row += 1
                def dataSection(typename, elements, keylist, dataRow, statsRow):

                    row = dataRow
                    sheet.write_string(row, 0, typename)
                    col = 2
                    for key in keylist:
                        sheet.write_string(row, col, key)
                        col += 1
                    row += 1
                    dataStart = row

                    sawData = {}
                    for id in elements:
                        col = 1
                        sheet.write_string(row, col, id)
                        col += 1

                        for key in keylist:
                            try:
                                v = data[key][id]
                                if key in ('order',):
                                    sheet.write_number(row, col, v, intFormat)
                                else:
                                    sheet.write_number(row, col, v, twoDecimal)
                                try:
                                    sawData[key] += 1
                                except:
                                    sawData[key] = 1

                            except:
                                pass
                            col += 1
                        row += 1

                    dataEnd = row - 1

                    row = statsRow
                    # insert link to first associated data entry
                    sheet.write_url(row, 0, "internal:'%s'!%s" %(
                        sheet.get_name(),
                        xl_rowcol_to_cell(dataStart, 0)),
                                    string=typename)
                    col = 2
                    for key in keylist:
                        if key not in ('order', ):
                            sheet.write_string(row, col, key)
                        col += 1
                    row += 1
                    sheet.write_string(row, 1, 'total')
                    sheet.write_string(row+1, 1, 'avg')
                    sheet.write_string(row+2, 1, 'stddev')
                    insertStats(keylist, sawData, statsRow + 1, statsRow + 2,
                                statsRow+3, dataStart, dataEnd, 2)
                    return dataEnd + 1

                chunkStatsRow = row
                fileStatsRow = chunkStatsRow + 4
                filterStatsRow = fileStatsRow + 4;

                if args.show_filter:
                    chunkDataRow = filterStatsRow + 4
                else:
                    chunkDataRow = fileStatsRow + 4
                    fileStatsRow = row
                parallelSumRow = row+1
                parallelSumCol = 3

                # first the chunk data...
                # process: time from immediately before fork in parent
                #          to immediately after 'process_one_file' in
                #          child (can't record 'dumper' call time
                #          because that also dumps the profile
                # child:   time from child coming to life after fork
                #          to immediately afer 'process_one_file'
                # exec: time take to by 'gcov' call
                # merge: time to merge child process (undump, read
                #       trace data, append to summary, etc.)
                # undump: dumper 'eval' call + stdout/stderr recovery
                # parse: time to read child tracefile.info
                # append: time to merge that into parent master report
                try:
                    chunks = sorted(data['child'].keys(), key=int, reverse=True)
                    row = dataSection('chunks', chunks, geninfoChunkKeys,
                                      chunkDataRow, chunkStatsRow)
                    row += 1
                    fileDataRow = row + 1
                    parallelSumCol = 2
                except:
                    # no chunk data - so just insert file data
                    fileStatsRow = chunkStatsRow
                    fileDataRow = fileStatsRow + 4
                    pass


                def cmpFile(a, b):
                    idA = int(data['order'][a])
                    idB = int(data['order'][b])
                    if idA < idB:
                        return 1
                    else:
                        return 0 if idA == idB else -1

                row = dataSection('files', sorted(data['file'].keys(), key=cmp_to_key(cmpFile)),
                                  geninfoKeys, fileDataRow, fileStatsRow)

                # now the fiter data - if any
                if args.show_filter:
                    filterDataRow = row + 1;
                    try:
                        chunks = sorted(data['filt_child'].keys(), key=int, reverse=True)
                        row = dataSection('filter', chunks, filterKeys,
                                          filterDataRow, filterStatsRow)
                        row += 1

                    except:
                        pass


                effectiveParallelism = "+%(sum)s/%(total)s" % {
                    'sum': xl_rowcol_to_cell(parallelSumRow, parallelSumCol),
                    'total': total,
                }
                sheet.write_formula(specialsStart + geninfoSpecialKeys.index('parallel'),
                                    1, effectiveParallelism, twoDecimal)

                if summarySheet:
                    summarySheet.write_string(summaryRow, 0, name)
                    # href to the corresponding page..
                    summarySheet.write_url(summaryRow, 0, "internal:'%s'!A1" % (
                        sheet.get_name()))
                    summaryCol = 1;

                    sheetRef = "='" + sheet.get_name() + "'!"

                    # insert total time and observed parallelism for this
                    # geninfo call
                    specialsRow = specialsStart
                    for k in geninfoSpecialKeys:
                        cell  = xl_rowcol_to_cell(specialsRow, 1)
                        summarySheet.write_formula(summaryRow, summaryCol,
                                                   sheetRef + cell, twoDecimal)
                        summaryCol += 1
                        specialsRow += 1

                    # now label this sheet's columns
                    #  and also insert reference to total time and average time
                    #  for each step into the summary sheet.
                    statsTotalRow = chunkStatsRow + 1
                    statsAvgRow = chunkStatsRow + 2
                    sections = [(geninfoChunkKeys, chunkStatsRow + 1, chunkStatsRow + 2),
                              (geninfoKeys, fileStatsRow + 1, fileStatsRow + 2),]
                    if args.show_filter:
                        sections.append((filterKeys, filterStatsRow +1,
                                         filterStatsRow+2))
                    for d in (sections):
                        totRow = d[1]
                        avgRow = d[2]
                        col = 2
                        for k in d[0]:
                            if k not in ('order',):
                                sum = xl_rowcol_to_cell(totRow, col)
                                summarySheet.write_formula(summaryRow, summaryCol,
                                                           sheetRef + sum, twoDecimal)
                                summaryCol +=1
                                avg = xl_rowcol_to_cell(avgRow, col)
                                summarySheet.write_formula(summaryRow, summaryCol,
                                                           sheetRef + avg, twoDecimal)
                                summaryCol +=1
                            col += 1
                continue

            elif tool == 'genhtml':

                for k in ('parse_source', 'parse_diff',
                          'parse_current', 'parse_baseline'):
                    if k in data:
                        sheet.write_string(row, 0, k)
                        sheet.write_number(row, 1, data[k], twoDecimal)
                        row += 1

                # total: time from start to end of the particular unit -
                # child: time from start to end of child process
                # annotate: annotate callback time (if called)
                # load:  load source file (if no annotation)
                # synth:  generate file content (no annotation and no no file found)
                # categorize: compute owner/date bins, differenntial categories
                # process:  time to generate data and write HTML for file
                # synth:  generate file content (no file found)
                # source:
                genhtmlKeys = ('total', 'child', 'annotate', 'synth', 'categorize', 'source', 'check_version', 'html')
                col = 3
                for k in genhtmlKeys:
                    sheet.write_string(row, col, k)
                    col += 1
                row += 1
                sumRow = row
                sheet.write_string(row, 2, "total")
                row += 1
                avgRow = row
                sheet.write_string(row, 2, "average")
                row += 1
                devRow = row
                sheet.write_string(row, 2, "stddev")
                row += 1

                #print(" ".join(data.keys()))
                try:
                    fileData = data['file']
                except:
                    print("%s:  incomplete data - skipping" % (name))
                    continue
                begin = row
                sawData = {}
                sawData['total'] = 0
                def printDataRow(name):
                    col = 4
                    for k in genhtmlKeys[1:]:
                        if (k in data and
                            name in data[k]):
                            try:
                                sheet.write_number(row, col, float(data[k][name]), twoDecimal)
                                if k in sawData:
                                    sawData[k] += 1
                                else:
                                    sawData[k] = 1
                            except:
                                print("%s: failed to write %s" %(name, data[k][name]))
                        col += 1

                def visitScope(f, dirname):
                    pth, name = os.path.split(f)
                    if None != dirname or pth != dirname:
                        return
                    sheet.write_string(row, 2, name)
                    sheet.write_number(row, 3, fileData[f], twoDecimal)
                    sawData['total'] += 1
                    printDataRow(f)
                    row += 1

                try:
                    dirData = data['dir']
                except:
                    # hack - 'flat' report doens't have directory data
                    for f in sorted(fileData.keys()):
                      visitScope(f, None)
                    dirData = {}

                for dirname in sorted(dirData.keys()):
                    sheet.write_string(row, 0, "directory")
                    sheet.write_string(row, 1, dirname)
                    sheet.write_number(row, 3, dirData[dirname], twoDecimal)
                    #pdb.set_trace()
                    printDataRow(dirname)
                    row += 1

                    start = row

                    for f in sorted(fileData.keys()):
                      visitScope(f, dirname)

                insertStats(genhtmlKeys, sawData, sumRow, avgRow, devRow, begin,
                           row-1, 3)

                overallParallelism = "+%(from)s/%(total)s" % {
                    'from': xl_rowcol_to_cell(sumRow, 3),
                    'total': total,
                    }
                sheet.write_formula(totalRow,2, overallParallelism, twoDecimal);
                continue

            for k in data:
                if k in ('parse_source', 'parse_diff',
                         'emit', 'parse_current', 'parse_baseline'):
                    sheet.write_string(row, 0, k)
                    sheet.write_number(row, 1, data[k], twoDecimal)
                    row += 1
                elif k in ('file', 'dir', 'load', 'synth', 'check_version',
                           'annotate', 'parse', 'append', 'segment', 'undump',
                           'merge', 'gen_info', 'data', 'graph', 'find'):
                    sheet.write_string(row, 0, k)
                    d = data[k]
                    for n in sorted(d.keys()):
                        sheet.write_string(row, 1, n)
                        try:
                            sheet.write_number(row, 2, float(d[n]), twoDecimal)
                        except:
                            print("%s: failed to write %s for [%s][%s]" %(name, str(d[n]), k, n))
                        row += 1;
                    continue
                elif k in ('config', 'overall', 'total'):
                    continue
                else:
                    print("not sure what to do with %s" % (k))

        if summarySheet:
            if len(geninfoSheets) < 2:
                summarySheet.hide()

            # insert the average and variance data...
            #  (there will not be any such data if we didn't run geninfo)
            try:
                col = 1
                lastSummaryRow = firstSummaryRow + len(geninfoSheets) - 1
                avgRow = 1
                devRow = 2
                firstCol = col
                for k in (*geninfoChunkKeys, *geninfoKeys):
                    if k in ('order',):
                        continue
                    for j in ('sum', 'avg'):
                        f = xl_rowcol_to_cell(firstSummaryRow, col)
                        t = xl_rowcol_to_cell(lastSummaryRow, col)
                        avg = "+AVERAGE(%(from)s:%(to)s)" % {
                            'from': f,
                            'to': t,
                        }
                        summarySheet.write_formula(avgRow, col, avg, twoDecimal)
                        avgCell = xl_rowcol_to_cell(avgRow, col)
                        dev = "+STDEV(%(from)s:%(to)s)" % {
                            'from': f,
                            'to': t,
                        }
                        summarySheet.write_formula(devRow, col, dev, twoDecimal)
                        col += 1
                insertConditional(summarySheet, avgRow, devRow,
                                  firstSummaryRow, firstCol, lastSummaryRow, col -1)
            except:
                pass
        s.close()

if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog="""
Simple utility to turn genhtml/geninfo/lcov "profile" JSON output files into a somewhat readable spreadsheet for easier analysis.

Example usage:
  $ spreadsheet.py -o foo.xlsx data.json data2.json data3.json ...
""")

    parser.add_argument("-o", dest='out', action='store',
                        default='stats.xlsx',
                        help='save excel to file')
    parser.add_argument("--threshold", dest='thresholdPercent', type=float,
                        help="difference from average smaller than this percentage is ignored (not colorized).  Default %0.2f" % (thresholdPercent))
    parser.add_argument("--low", dest='devMinThreshold', type=float,
                        help="difference from average larger than this * stddev colored yellow.  Default: %0.2f" %(devMinThreshold))
    parser.add_argument("--high", dest='devMinThreshold', type=float,
                        help="difference from average larger than this * stddev colored red.  Default: %0.2f" %(devMaxThreshold))
    parser.add_argument('-v', '--verbose', dest='verbose', default=0,
                        action='count', help='verbosity of report: more data');
    parser.add_argument('--show-filter', dest='show_filter', default=False,
                        action='store_true', help='include filter keys in table');

    parser.add_argument('files', nargs=argparse.REMAINDER)

    try:
        args = parser.parse_args()
    except IOError as err:
        print(str(err))
        sys.exit(2)

    GenerateSpreadsheet(args.out, args.files, args)
