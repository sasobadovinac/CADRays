# python 2.7.13

# examples
#  run tests
#    python cadrays_testing.py -i d:/scripts -f 100 -m d:/scripts/template
#  update template results
#    python cadrays_testing.py -o d:/scripts -m d:/scripts/template -u

import sys
import getopt
import re

from datetime import datetime
from os import listdir, system, remove, rename, makedirs
from os.path import isfile, join, isdir, splitext, abspath, exists
from HTMLParser import HTMLParser
from shutil import copyfile
from PIL import Image, ImageChops

class MyHTMLParser(HTMLParser):
    file = ""
    result = []
    def handle_data(self, data):
        if data.startswith("File"):
            self.file = data[data.find(" ") + 1 :]
        elif data.startswith("Framerate"):
            self.result.append((self.file, data[data.find("=") + 2 : data.find("fps") - 1]))

def createHTML(outputFile, date, results, model, images):
    outputFile.write("<html><head><title>Result</title></head><body>" + "\n")
    outputFile.write("<h1>" + date.strftime("%d/%m/%Y %H:%M:%S") + "</h1>" + "\n\n")

    outputFile.write("<ol>" + "\n")
    for result in results:
        outputFile.write("<li>" + "\n")
        outputFile.write("<p style=\"font-size:25px\"><strong>File " + result[0] + "</strong></p>" + "\n")
        outputFile.write("<ul>" + "\n")
        outputFile.write("<li>" + "\n")
        framerate = result[1]
        matches = [b for (a,b) in model if a == result[0]]
        if len(matches) > 0:
            diff = (float(framerate) / float(matches[0]) - 1) * 100
            if abs(diff) <= maxDiff:
                outputFile.write("<p style=\"font-size:20px\"><strong>Framerate = " + framerate + " fps (prev = " +
                                 matches[0] + ") [" + ("%+.4f" % diff) + "%]</strong></p>" + "\n")
            elif diff > 0:
                outputFile.write("<p style=\"font-size:20px\"><strong><span style=\"background-color:green\">Framerate = " +
                                 framerate + " fps (prev = " + matches[0] + ") [" + ("%+.4f" % diff) + "%]</span></strong></p>" + "\n")
            else:
                outputFile.write("<p style=\"font-size:20px\"><strong><span style=\"background-color:red\">Framerate = " +
                                 framerate + " fps (prev = " + matches[0] + ") [" + ("%+.4f" % diff) + "%]</span></strong></p>" + "\n")
        else:
            outputFile.write("<p style=\"font-size:20px\"><strong>Framerate = " + framerate + " fps</strong></p>" + "\n")
        outputFile.write("</li>" + "\n")
        outputFile.write("</ul>" + "\n")
        outputFile.write("</li>" + "\n")
    for image in images:
        outputFile.write("<li>" + "\n")
        outputFile.write("<p style=\"font-size:25px\"><strong>File " + image[0] + "</strong></p>" + "\n")
        if image[2] != "":
            outputFile.write("<table><tr><th>Output result</th><th>Model result</th><th>Difference</th></tr>" + "\n")
            outputFile.write("<tr><td><img src=\"file://" + image[1] + "\" width=\"100%\" height=\"100%\"></td>" +
                             "<td><img src=\"file://" + image[2] + "\" width=\"100%\" height=\"100%\"></td>" +
                            "<td><img src=\"file://" + image[3] + "\" width=\"100%\" height=\"100%\"></td></tr></table>" + "\n")
        elif image[1] != "":
            outputFile.write("<table><tr><th>Output result</th></tr>" + "\n")
            outputFile.write("<tr><td><img src=\"file://" + image[1] + "\" width=\"100%\" height=\"100%\"></td></tr></table>" + "\n")
        outputFile.write("</li>" + "\n")
    outputFile.write("</ol>" + "\n")

usageString = "-h       :  Show help\n" \
              "-i  arg  :  Path to the folder with scripts\n" \
              "-c  arg  :  Path to CADRays.exe\n" \
              "-f  arg  :  Frame number\n" \
              "-d  arg  :  Percentage min difference at which strings will be selected\n" \
              "-o  arg  :  Path to the output folder\n" \
              "-m  arg  :  Path to the folder with results for comparing\n" \
              "-u       :  Update the template result with the last test"
inputFolder = ''
outputFolder = ''
cadraysPath = "CADRays.exe"
modelFolder = ""
framesNum = 100
maxDiff = 2
updateModel = False

try:
    opts, args = getopt.getopt(sys.argv[1:],"hi:c:f:d:o:m:u")
except getopt.GetoptError:
    print usageString
    sys.exit(2)
for opt, arg in opts:
    if opt == '-h':
        print usageString
        sys.exit()
    elif opt == "-u":
        updateModel = True
    elif opt == "-i":
        inputFolder = arg
    elif opt == "-o":
        outputFolder = arg
    elif opt == "-c":
        cadraysPath = arg
    elif opt == "-m":
        modelFolder = arg
    elif opt == "-f":
        try:
            framesNum = int(arg)
        except ValueError:
            print "Incorrect input -f : \"int\""
            sys.exit(2)
    elif opt == "-d":
        try:
            maxDiff = int(arg)
        except ValueError:
            print "Incorrect input -d : \"int\""
            sys.exit(2)

if (inputFolder == "" or not isdir(inputFolder)) and updateModel == False:
    print "Path to scripts folder is incorrect"
    sys.exit(2)

if (cadraysPath == "" or not isfile(cadraysPath)) and updateModel == False:
    print "Path to CADRays.exe is incorrect"
    sys.exit(2)

if modelFolder == "" or not isdir(modelFolder):
    print "Path to the folder with results for comparing is incorrect"
    sys.exit(2)

if outputFolder == "":
    if updateModel == False:
        outputFolder = inputFolder
    else:
        print "Path to output folder is incorrect"
        sys.exit(2)
elif not isdir(outputFolder):
    print "Path to output folder is incorrect"
    sys.exit(2)

outputFolder = abspath(outputFolder)
modelFolder = abspath(modelFolder)

if updateModel == True:
    folders = [folders for folders in listdir(outputFolder) if isdir(outputFolder + "/" + folders)]
    maxDate = datetime(1970, 01, 01, 0, 0)
    for folder in folders:
        if re.match("(\d\d?)_(\d\d?)_(\d\d\d\d) (\d\d?)_(\d\d?)_(\d\d?)", folder):
            date = datetime.strptime(folder, "%d_%m_%Y %H_%M_%S")
            if maxDate < date:
                maxDate = date
    if maxDate == datetime(1970, 01, 01, 0, 0):
        print "No results founded"
        sys.exit(2)
    parser = MyHTMLParser()
    parser.feed(open(join(outputFolder, maxDate.strftime("%d_%m_%Y %H_%M_%S"), "Result.html")).read())
    print parser.result
    if isfile(join(modelFolder, "Result.html")):
        remove(join(modelFolder, "Result.html"))
    modelFile = open(join(modelFolder, "Result.html"), "w")
    createHTML(modelFile, maxDate, parser.result, [], [])
    modelFile.close()
    for file in listdir(join(outputFolder, maxDate.strftime("%d_%m_%Y %H_%M_%S"))):
        print file
        match = re.match("Output_(.*)_(.*)\.png", file)
        if match:
            copyfile(join(outputFolder, maxDate.strftime("%d_%m_%Y %H_%M_%S"), file), join(modelFolder, match.group(1) + ".png"))
else:
    inputFolder = abspath(inputFolder)
    cadraysPath = abspath(cadraysPath)

    outputs = [files for files in listdir(inputFolder) if isfile(join(inputFolder, files))
               and re.match(r'Output_(.*)\.((txt)|(png))', files)]
    for output in outputs:
        remove(inputFolder + "/" + output)

    scripts = [files for files in listdir(inputFolder) if isfile(join(inputFolder, files))
            and splitext(files)[1].upper() == ".TCL"]

    for script in scripts:
        system("start /wait cmd /c " + cadraysPath + " " + inputFolder + "/" + script + " " + str(framesNum))
        while True:
            outputFile = join(inputFolder, "Output_" + splitext(script)[0] + "_" + str(framesNum) + ".txt")
            if isfile(outputFile):
                break


    date = datetime.now()
    dateStr = date.strftime("%d_%m_%Y %H_%M_%S")

    if isfile(join(modelFolder, "Result.html")):
        modelFile = open(join(modelFolder, "Result.html"), "r")
        parser = MyHTMLParser()
        parser.feed(modelFile.read())
        modelResult = parser.result
    else:
        modelResult = []

    resultsFiles = [files for files in listdir(inputFolder) if isfile(join(inputFolder, files))
               and re.match(r'Output_(.*)\.txt', files)]
    resultsFiles.sort()

    if not exists(outputFolder + "/" + dateStr):
        makedirs(outputFolder + "/" + dateStr)

    for file in listdir(inputFolder):
        if re.match("Output_(.*)\.png", file):
            rename(inputFolder + "/" + file, outputFolder + "/" + dateStr + "/" + file)

    results = []
    images = []
    for resultFile in resultsFiles:
        fileName = resultFile[resultFile.find("_") + 1 : resultFile.rfind("_")] + ".tcl"
        file = open(join(inputFolder, resultFile), "r")
        framerate = file.readline()
        results.append((fileName, framerate))
        file.close()
        resultImagePath = join(outputFolder, dateStr, splitext(resultFile)[0] + ".png")
        modelImagePath = join(modelFolder, splitext(fileName)[0] + ".png")
        diffImagePath = ""
        if not isfile(resultImagePath):
            resultImagePath = ""
        if not isfile(modelImagePath):
            modelImagePath = ""
        if not resultImagePath == "" and not modelImagePath == "":
            resultImage = Image.open(resultImagePath, "r")
            modelImage = Image.open(modelImagePath, "r")
            diffImage = ImageChops.difference(resultImage, modelImage).convert('L').point(lambda x: 0 if x == 0 else 255, '1')
            diffImagePath = join(outputFolder, dateStr, "Diff_" + splitext(fileName)[0] + ".png")
            diffImage.save(diffImagePath)
        images.append((fileName, resultImagePath, modelImagePath, diffImagePath))

    print images

    outputFile = open(join(outputFolder, dateStr, "Result" + ".html"), "w")
    createHTML(outputFile, date, results, modelResult, images)
    outputFile.close()

    outputs = [files for files in listdir(inputFolder) if isfile(join(inputFolder, files))
               and re.match(r'Output_(.*)\.txt', files)]
    for output in outputs:
        remove(join(inputFolder, output))

print "Done"