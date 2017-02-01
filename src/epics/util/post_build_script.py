#!/usr/bin/env python
# post_build_script.py
# This script is assumed to be run in the advLigoRTS directory and be given a model name.
# The model is assumed to be a .mdl and lives in advLigoRTS/src/epics/simLink/
# It assumes any libraries of parts also live in that directory.
# It will then parse the .mdl file to determine if any post build screens need to be generated or if any scripts need to be run.  These scripts will be called with the site, ifo, the name of the part or subsystem, and then any further commands in the description of the part.


# Joseph Betzwieser, May 2nd, 2011

import os, sys
import re
import string

error = False
if sys.argv[1] == "-q":
  model_name = sys.argv[2]
  quiet_mode = True
else:
  model_name = sys.argv[1]
  quiet_mode = False


ifo_initial = model_name[0:2]
site_initial = model_name[0]
global top_names
top_names = None
global model_params
model_params = ''

def find_file_in_path(env_path,file_name):
  for path in env_path.split(':'):
    full_file_path = path + '/' +  file_name
    if os.path.isfile(full_file_path):
      return full_file_path
  sys.stderr.write("ERROR: Could not find file: " + file_name + "\n")
  sys.stderr.write("Searched path: " + env_path + "\n")
  sys.stderr.write("Exiting\n")
  sys.exit(1)


#Digital filter simulink reference which doesn't have an easily tracable .mdl file
#Simply don't follow the reference link.

simulink_reference=["dsparch4","simulink"]


print ""
#Grab some basic env information to be able to set default paths
try:
  site = os.environ['site']
except KeyError:
  sys.stderr.write("ERROR: No environment variable 'site' defined\n")
  error = True

try:
  ifo = os.environ['IFO']
except KeyError:
  sys.stderr.write("ERROR: No environment variable IFO defined\n")
  error = True

if error:
  sys.stderr.write("Exiting due to ERRORs\n")
  sys.exit(1)

#Default Paths
#FIX ME: JCB
rcg_lib_path = os.path.join('/opt/rtcds',site,ifo.lower(),'core/release/src/epics/simLink/lib/') + ":" + os.path.join('/opt/rtcds',site,ifo.lower(),'core/release/src/epics/simLink/')

medm_target = os.path.join('/opt/rtcds',site,ifo.lower(),'medm',model_name)

cds_medm_path = '/'.join(['/opt/rtcds',site,ifo.lower(),'medm/templates/']) 
cds_scripts_path = '/'.join(['/opt/rtcds',site,ifo.lower(),'/scripts/post_build/'])

tmp = model_name + 'epics/burt'
epics_sdf_file = '/'.join(['/opt/rtcds',site,ifo.lower(),'target',model_name,tmp,'safe.snap'])
tmp = model_name + 'epics'
epics_burt_file = '/'.join(['/opt/rtcds',site,ifo.lower(),'target',model_name,tmp,'autoBurt.req'])

#Try to update default paths with actual environment variables

try:
  rcg_lib_path = os.environ['RCG_LIB_PATH']
except KeyError:
  sys.stderr.write("No RCG_LIB_PATH defined, using default path:\n")
  sys.stderr.write(rcg_lib_path + "\n\n")

try:
  cds_medm_path = os.environ['CDS_MEDM_PATH']
except KeyError:
  sys.stderr.write("No CDS_MEDM_PATH defined, using default path:\n")
  sys.stderr.write(cds_medm_path + "\n\n")

try:
  cds_scripts_path = os.environ['CDS_SCRIPTS_PATH']
except KeyError:
  sys.stderr.write("No CDS_SCRIPTS_PATH defined, using default path:\n")
  sys.stderr.write(cds_scripts_path + "\n\n")

try:
  medm_target = os.environ['CDS_MEDM_TARGET']
except KeyError:
  if quiet_mode == False:
    sys.stderr.write("No CDS_MEDM_TARGET defined, using default path:\n")
    sys.stderr.write(medm_target + "\n\n")

#Get the actual model file
full_model_path = find_file_in_path(rcg_lib_path,model_name + '.mdl')  

#An object that contains all the Name "C1SY", Descrption "Blah, blah" in a dictionary
#Also contains a subarray of blocks underneath it
class Block:
  def __init__(self):
    self.data = {}
    self.subblocks = []
    
#Parses a "block" of an .mdl file and returns the corresponding Block structure along with the last line number parsed
def parse_block(data_lines,line_number,reference_name):
  new_block = Block()
  ###Normal, non-reference parsing
  if reference_name == None:
    new_block.data['MyBlockType'] = data_lines[line_number].split()[0].strip()
    line_number += 1
    while data_lines[line_number].strip() != '}':
      current_line = data_lines[line_number]
      #We encounter a new block definition so start a new node
      if current_line.split()[-1].strip() == '{':
        temp_block, line_number = parse_block(data_lines,line_number,None)
        #Check to see if we've found a reference block
        try:
          if (temp_block.data['BlockType'] == 'Reference') &  (not (simulink_reference[0] in temp_block.data['SourceBlock'])) & (not (simulink_reference[1] in temp_block.data['SourceBlock'])):
            #If we did find a reference block, get the reference data
            current_name = temp_block.data['Name']
            library_lines = find_library(temp_block.data['SourceBlock'])
            scratch_block, scratch = parse_block(library_lines,0,temp_block.data['SourceBlock'])
            #Keep the farthest back reference description for screen generation as well - allows changes to just the library part
            if ('Description' in scratch_block.data.keys()):
              if not ('Reference_Descrip' in scratch_block.data.keys()):
                scratch_block.data['Reference_Descrip'] = [scratch_block.data['Description']]
              else:
                scrach_block.data['Reference_Descrip'].append(scratch_block.data['Description'])
            #Overwrite reference block data with data from the calling block (i.e. 'Name', etc)
            if not ('Description' in temp_block.data.keys()):
              temp_block.data['Description'] = ""
            for data_key in temp_block.data.keys():
              scratch_block.data[data_key] = temp_block.data[data_key]
            temp_block = scratch_block
        except KeyError:
          pass
        new_block.subblocks.append(temp_block)
      #We have just a line with data      
      else:
        new_data_entry = data_lines[line_number]
        #Handle the case of descriptions/inputs which are multiple lines, with 2nd and later starting with '"'.
        #Reuse the last entry name since that will be the Description line still
        if new_data_entry.strip()[0] == '"':
          new_block.data[entry_name] = new_block.data[entry_name].rstrip('"') + new_data_entry.lstrip().lstrip('"').rstrip()
          #Handle normal data entry
        else:
          #HACK:
          #FIX ME: JCB  Do something correct for the $ in names
          entry_name = new_data_entry.split()[0].strip('$')
          entry_data = re.search(entry_name + '(.*)',new_data_entry).group(1).strip()
          new_block.data[entry_name] = entry_data
        line_number += 1
    #We've reached the end of the block, so we return the block plus line_number for the next line     
    return new_block, line_number+1
  #### End normal parsing
  ### Reference parsing starts here
  ### We've been given a reference name, so we go and try to find it, ignoring line_number      
  else:
    #Remove the first section, which is the file name
    reference_part = re.search('/(.*)',reference_name).group(1)
    reference_name_tree = []
    #Now parse the rest to get the individual block names
    while True:
      search_result = re.search('(.*/(?!/))(.*)',reference_part)
      if search_result == None:
        reference_name_tree.append(reference_part.strip('"'))
        break
      reference_name_tree.append(search_result.group(1).strip('/'))
      reference_part = search_result.group(2)

    #Search the file (i.e. data_lines) for the names in order
    for name in reference_name_tree:
      for current_line_count in range(line_number,len(data_lines)):
	current_line = data_lines[current_line_count]
        if (current_line.split()[0] == 'Name'):
          if (current_line.split('"')[1] == name):
            break
    #We've found all the names, and must currently be inside the correct block,
    #Go back to find the description block
    descrip_line_count = current_line_count
    description_present = True
    while data_lines[descrip_line_count].split()[0].strip() != 'Description':
      descrip_line_count += 1
      if descrip_line_count == len(data_lines):
        description_present = False
        break
    if description_present:
      entry_name = data_lines[descrip_line_count].split()[0]
      entry_data = re.search(entry_name + '(.*)',data_lines[descrip_line_count]).group(1).strip()
      temp_description = entry_data

    #Now go forward until we find the opening {
    #We go forward because of the way the name is used twice - we're interested in the interior info.
    #Then do the usual parse.
    while data_lines[current_line_count].split()[-1] != '{':
      current_line_count += 1
      if current_line_count == len(data_lines):
	sys.stderr.write("ERROR: For part: " + reference_name + "\n")
	sys.stderr.write("Could not find the proper library reference.\n")
	sys.stderr.write("Your model may be referencing a different source model than what is in the current library path.\n\n")
	sys.stderr.write("Current path is: " + rcg_lib_path + "\n\n")
  	sys.stderr.write("Exiting\n")
  	sys.exit(1)

    new_block,scratch = parse_block(data_lines,current_line_count,None)
    if description_present:
      new_block.data['Description'] = temp_description
    return new_block, scratch
  #### End reference parsing

#This function goes and finds the location of part's corresponding library .mdl
#It then returns all lines from the library .mdl file.
def find_library(library_name):
  reference_file_name = re.search('([^\/]*)',library_name).group(1).strip('"').strip('/')
  #reference_file_name = re.search('(.*/(?!/))(.*)',library_name).group(1).strip('"').strip('/')
  for path in rcg_lib_path.split(':'):
    full_reference_path = path + '/' + reference_file_name + '.mdl'
    if os.path.isfile(full_reference_path):
      try:
        reference_file = open(full_reference_path,'r')
      except:
        sys.stderr.write("ERROR: For part referencing: " + library_name + "\n")
        sys.stderr.write("Could not open reference file: " + reference_file_name + "\n")
        sys.stderr.write("Exiting\n")
        sys.exit(1)
      return reference_file.readlines()    
  sys.stderr.write("ERROR: For part referencing: " + library_name + "\n")
  sys.stderr.write("Could not find reference file: " + reference_file_name + "\n")
  sys.stderr.write("Exiting\n")
  sys.exit(1)

#This function is the top level function which recursively goes through the mdl file
def parse_simulink_file(data_lines):
  line_number = 0
  root = Block()
  root.data['MyBlockType'] = 'File'
  while (line_number < len(data_lines)-1):
    temporary_block, line_number = parse_block(data_lines, line_number,None)
    root.subblocks.append(temporary_block)
  return root



#Function which goes through the completed tree and looks for keywords
def read_tree(node,name_so_far):
  #Are we a node with Name information
  if 'Name' in node.data:
    #If so, are we of a simulink "Block" type?
    if node.data['MyBlockType'] in ['Block','SubSystem']:
      if 'Tag' in node.data:
        if 'top_names' in node.data['Tag']:
          if len(name_so_far) == 1:
            name_so_far = (node.data['Name'].strip('"'),)
          else:
            name_so_far = (node.data['Name'].strip('"'),) + name_so_far[1:len(name_so_far)]
        else:
          name_so_far = name_so_far + (node.data['Name'].strip('"'),)
      else:
        name_so_far = name_so_far + (node.data['Name'].strip('"'),)
    
    chan_name = ''
    part_name = ''
    if (len(name_so_far) == 0):
      pass
    elif (len(name_so_far) == 1):
      chan_name = ifo.upper() + ':' + name_so_far[0]
      part_name = ifo.upper() + name_so_far[0]
    elif (len(name_so_far) == 2):
      chan_name = ifo.upper() + ':' + name_so_far[0] + '-' + name_so_far[1]
      part_name = ifo.upper() + name_so_far[0] + '_' + name_so_far[1]
    elif (len(name_so_far) > 2):
      temp_name = ifo.upper() + ':' + name_so_far[0] + '-' + name_so_far[1]
      chan_name = '_'.join((temp_name,) + name_so_far[2:len(name_so_far)])
      temp_name = ifo.upper() + name_so_far[0]
      part_name = '_'.join((temp_name,) + name_so_far[1:len(name_so_far)])
  
    #Arrays containing lines with keywords at the beginning of the line
    new_script_line = []
    new_adl_line = []
    script_line = []
    adl_line = []
    #Override of library link screen generation
    make_library_screens = True

    # Check the Description field for key words
    if 'Description' in node.data:
      for line in node.data['Description'].lstrip('"').rstrip('"').split('\\n'):
        if (re.search('^SCRIPT=',line) != None):
          new_script_line.append(re.search('^SCRIPT=(.*)',line).group(1))
        if (re.search('^ADL=',line) != None):
          new_adl_line.append(re.search('^ADL=(.*)',line).group(1))
        if (re.search('^NO DEFAULT',line) != None):
          make_library_screens = False  
          print "No default screens"

    if ('Reference_Descrip' in node.data) and (make_library_screens):
      for x in range(len(node.data['Reference_Descrip'])):
        for line in node.data['Reference_Descrip'][len(node.data['Reference_Descrip'])-1-x].lstrip('"').rstrip('"').split('\\n'):
          if (re.search('^SCRIPT=',line) != None):
            script_line.append(re.search('^SCRIPT=(.*)',line).group(1))
          if (re.search('^ADL=',line) != None):
            adl_line.append(re.search('^ADL=(.*)',line).group(1))
 
    if new_script_line != []:
      for script_entry in new_script_line:
        script_line.append(script_entry)
    if new_adl_line != []:
      for adl_entry in new_adl_line:
        adl_line.append(adl_entry)

    dict_of_params = {}
    list_of_params = model_params.split('\\n')
    for param in list_of_params:
      try: 
        dict_of_params[param.split('=')[0]] = param.split('=')[1]
      except:
        pass

    #Default substitions for screens and script inputs
    default_subs = []
    default_subs.append(["#SYS#",name_so_far[0]])
    default_subs.append(["#CHANNEL#", chan_name])
    default_subs.append(["#FULL_PART_NAME#", part_name])
    default_subs.append(["#PART_NAME#", node.data['Name'].strip('"').strip()])
    default_subs.append(["#SITE#",site.upper()])
    default_subs.append(["#site#",site.lower()])
    default_subs.append(["#IFO#",ifo.upper()])
    default_subs.append(["#ifo#",ifo.lower()])
    default_subs.append(["#DCU_ID#",dict_of_params['dcuid']])
    default_subs.append(["#DATA_RATE#",dict_of_params['rate']])
    default_subs.append(["#TARGET_DIR#",medm_target])
    default_subs.append(["#MODEL_NAME#", model_name])
    for x in range(len(name_so_far)):
      default_subs.append(["#SYS[" + str(x- len(name_so_far)) + "]#",name_so_far[x-len(name_so_far)]])
      default_subs.append(["#SYS[" + str(x) + "]#",name_so_far[x]])

    #Run through each line found and do the approriate thing
    for script in script_line:
      for before, after in default_subs:
        script = string.replace(script,before,after)
      script_name = script.split(' ')[0]
      script_location = find_file_in_path(cds_scripts_path,script_name)
      script_command = script_location + script[len(script_name):]
      print script_command
      os.system(script_command)
      
    for adl in adl_line:
      custom_subs = []
      adl_info = adl.strip().split(',')
      adl_file_name = ''
      try:
        if len(adl_info) > 1:
          custom_pairs = adl_info[1:]
          for pair in custom_pairs:
            custom_subs.append(pair.split('='))
        adl_file_name = adl_info[0]
      except TypeError:
        adl_file_name = adl_info

      #Find the file
      adl_target_name = part_name + '.adl'
      found_file = False
      for path in cds_medm_path.split(':'):
        full_adl_path = path + '/' + adl_file_name
        if os.path.isfile(full_adl_path):
          found_file = True
          if True:
            template_file = open(full_adl_path,'r')
            temp_lines = template_file.readlines()
            template_file.close()

            change_name = False
            for before, after in default_subs:
              if '--name' in before:
                adl_target_name = after.strip()
                change_name = True
            for before, after in custom_subs:
              if '--name' in before:
                adl_target_name = after.strip()
                change_name = True
            if change_name:
              for before, after in default_subs:
                adl_target_name = string.replace(adl_target_name,before,after)
              for before, after in custom_subs:
                if not '--name' in before:
                  adl_target_name = string.repalce(adl_target_name,before,after) 

            adl_target_name = medm_target + '/' + adl_target_name

            for k in range(len(temp_lines)):
              for before, after in default_subs:
                temp_lines[k] = string.replace(temp_lines[k],before,after)
              for before, after in custom_subs:
                if not '--name' in before:
                  temp_lines[k] = string.replace(temp_lines[k],before,after)
	    if quiet_mode == False:
              print adl_target_name 
            output_medm_file = open(adl_target_name,'w')
            for k in range(len(temp_lines)):
              output_medm_file.write(temp_lines[k])
            break
          if False:
            sys.stderr.write("Error while reading from " + adl_file_name + " or writing to " + adl_target_name + "\n")
            break
      if found_file == False:
        sys.stderr.write("Unable to find the following file in CDS_MEDM_PATH: " + adl_file_name + "\n")

  if len(node.subblocks) != 0:
    for node in node.subblocks:
      read_tree(node, name_so_far)


############################  
#Start of the actual program

#Very first block, which will have the system name associated with it.
root_block = Block()

#Check to see if we can open the given model file, exit otherwise
try:
  mdl_file = open(full_model_path,'r')
except IOError:
  sys.stderr.write("Could not find or open: " + full_model_path + "\n")
  sys.exit(1)


#Grabs all the lines from the model .mdl file
mdl_data = mdl_file.readlines()
mdl_file.close()

#Begin parsing the model
root_block = parse_simulink_file(mdl_data)

#Figure out DCU_ID
#Find the cdsParameter block
def find_cdsParam(node):
  if 'Tag' in node.data:
    if 'cdsParameters' in node.data['Tag']:
      return node.data['Name']
  for subnode in node.subblocks:
    temp = find_cdsParam(subnode)
    if temp != None:
      return temp
  return None

model_params = find_cdsParam(root_block)

#Do something fancy with top names now
read_tree(root_block,(model_name[2:5].upper(),))
print epics_sdf_file
print epics_burt_file
if os.path.isfile(epics_sdf_file):
	print 'safe.snap exists '
else:
	print 'Creating safe.snap file'
	f = open(epics_burt_file,'r')
	sdf = open(epics_sdf_file,'w')
	for line in f:
		if '.HSV' in line or '.LSV' in line or '.HIGH' in line or '.LOW' in line or '.OSV' in line or '.ZSV' in line:
			continue
		word = line.split()
		if word[0] == 'RO':
			continue
		elif '_SW1S' in word[0]:
			tmp = word[0] + ' 1 4.000000000000000e+00 0xffffffff \n'  
			sdf.write(tmp)
		elif '_SW2S' in word[0]:
			tmp = word[0] + ' 1 1.536000000000000e+03 0xffffffff \n'  
			sdf.write(tmp)
		elif '_BURT_RESTORE' in word[0]:
			tmp = word[0] + ' 1 1.000000000000000e+00 1 \n'  
			sdf.write(tmp)
		elif '_DACDT_ENABLE' in word[0]:
			tmp = word[0] + ' 1 OFF 1 \n'  
			sdf.write(tmp)
		else:
			tmp = word[0] + ' 1 1.000000000000000e+00 1 \n'  
			sdf.write(tmp)
	f.close()
	sdf.close()


