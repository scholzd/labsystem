<?php
/**
 *  labsystem.m-o-p.de -
 *                  the web based eLearning tool for practical exercises
 *  Copyright (C) 2010  Marc-Oliver Pahl
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Helper functions used by ../pages/XIport.php
 *
 * @module     ../include/XIlib.inc
 * @author     Marc-Oliver Pahl, Meyyar Palaniappan
 * @version    2.0
 */

////////////////////////////// called externally /////////////////////////////////////////
/**
 * Creates the mapping array out of the array containing the existing element indices as vale.
 *
 * @param array $labElementArray  An array containing all elements to be exported as values.
 * @return                        Acts directly on the given array.
 */
function createIdMapping( &$labElementArray ){
	natsort( $labElementArray ); // sort values lexicographically
	//remove stared values as they describe not existing elements:
	$labElementArray = array_filter( $labElementArray, 'notStared' );
	$labElementArray = array_flip( $labElementArray ); // exchange keys and values (+ remove duplicate elements...)
	$lastSeenElement = '';
	$elementCounter = 99;
	foreach ($labElementArray as $key => $value){
		if ( $lastSeenElement != $key[0] ){ // new element ID
			$lastSeenElement = $key[0];
			$elementCounter = 1;
		}
		$labElementArray[ $key ] = $lastSeenElement.$elementCounter++;
	}
}

/**
 * creates the new empty elements for the imported lab elements and populates the reverse index mapping array
 *
 * @param  string $labFolder     	        Lab folder name on the disk inside the $cfg->get('exportImportDir')
 * @return array						              array with the reverse mapping index
 */
function createIdImportMappingInitDB( $labFolder ){
  global $cfg;
  $IdMapping = array();
  $labElementsOnDisk = scandir( $cfg->get('exportImportDir').$labFolder );
  foreach ( $labElementsOnDisk as $value ){
    if ( (strlen( $value ) < 4) || (substr($value, -4) != '.txt') ) continue; // skip all uninteresting files
        
    $elementType = $value[0]; // get type (first letter)
    $elementID = intval( substr($value, 1, 7) ); // get number (rest)

    $newIdx = createNew( $elementType ); // creates element in DB
    $IdMapping[$elementType.$elementID] = $elementType.$newIdx;
  }
  return $IdMapping;
}

/**
 * Persists the element given.
 *
 * @param object &$element     The element.
 * @param string $labUID       The unique identifier of this lab (UID for export).
 * @return string              Status information
 */
function persistElement( &$element, $labUID, $isImport=false ){
  if (!$isImport) { // EXPORT
  	$fileName = $element->elementId.substr( '0000000'.$element->idx, -7 ).'.txt';
  	fileWrite( $fileName, $element->getSerialized(), $labUID );
  
  	//debug: $element->initFromSerialized( $element->getSerialized() ); // identity!
  	return '<div class="labsys_mop_elements_menu_'.strtolower( $element->elementId ).'">'."\r\n".
  	htmlentities($element->title).
           ' <img src="../syspix/button_next_13x12.gif" width="13" height="12" border="0" alt="-&gt;" title="-&gt;"> '.
  	$fileName.
           ' <img src="../syspix/button_export2disk_30x12.gif" width="30" height="12" border="0" alt="export" title="export">'.
           "</div>\r\n";
  } else { // IMPORT
    $GLOBALS[ $element->elementId."DBI" ]->setData( $element );
  	return '<div class="labsys_mop_elements_menu_'.strtolower( $element->elementId ).'">'."\r\n".
  	htmlentities($element->title).
           ' <img src="../syspix/button_prev_13x12.gif" width="13" height="12" border="0" alt="&lt;-" title="&lt;-"> '.
  	$element->elementId.$element->idx.
           ' <img src="../syspix/button_importFromDisk_30x12.gif" width="30" height="12" border="0" alt="import" title="import">'.
           "</div>\r\n";
  }
}

/**
 * Writes the $content into $fileName into $cfg->get('exportImportDir').
 *
 * @param $fileNameWithPath    Path and Name of the file to be created.
 * @param $content             Content to be written in the file.
 * @param string $labUID       The unique identifier of this lab (UID for export).
 * @return
 */
function fileWrite( $fileName, $content, $labUID ) {
	global $cfg;
	$fileNameWithPath = $cfg->get('exportImportDir').$labUID.'/'.$fileName;
	if ( !file_exists(dirname($fileNameWithPath))) 
    mkdir(dirname($fileNameWithPath),0755 , true) or trigger_error("Error creating folder ".dirname($fileNameWithPath), E_USER_ERROR);
	$fh = fopen($fileNameWithPath, 'w') or trigger_error("Can't create/ open file ".$fileNameWithPath, E_USER_ERROR);
	fwrite($fh, $content);
	fclose($fh);
}

/**
 * Removes the example solutions from $HTMLcode.
 *
 * @param string  &$HTMLcode                The html code to be altered.
 * @return                                  Alters the code directly.
 */
function removeExampleSolutions(&$HTMLcode){ 
  // inputs
	$HTMLcode= preg_replace_callback( '#(?<=<!-- example solution -->)'. // Assertion "must be preceded by"
                                    '(.*)'.
                                    '(?=<!-- /example solution -->)'.
                                    '#msU',
                                      create_function(
                                          '$matches',
                                          'return "<br><br><br><br>";'
                                      ), 
                                   $HTMLcode
                                   );
  // MCs
  // remove checked boxes
	$HTMLcode= str_replace(' checked="checked"', '', $HTMLcode);
  //remove answer explanations
	$HTMLcode= preg_replace_callback( '#(<div class="labsys_mop_m_answer_explanation">'. // Assertion "must be preceded by"
                                    '.*'.
                                    '</div>)'.
                                    '#msU',
                                      create_function(
                                          '$matches',
                                          'return "";'
                                      ), 
                                   $HTMLcode
                                   );
  // remove disabled (just for fun)
	$HTMLcode= str_replace(' disabled="disabled" ', '', $HTMLcode);
  // remove graying showing right solutions
	$HTMLcode= preg_replace_callback( '#(<span class="labsys_mop_grayed">)'. // Assertion "must be preceded by"
                                    '(.*)'.
                                    '(</span>)'.
                                    '#msU',
                                      create_function(
                                          '$matches',
                                          'return $matches[2];'
                                      ), 
                                   $HTMLcode
                                   );
  // remove graying showing right solutions
	$HTMLcode= preg_replace_callback( '#(<a href="/pages/XIport.php\?)(\S+)#',
                                      create_function(
                                          '$matches',
                                          'return "...";'
                                      ), 
                                   $HTMLcode
                                   );               

                                   

}

/**
 * Processes the $content:
 *   - reindexes linked elements
 *   - relocates images to $IMAGE_PATH
 *   - adds images to $filesToBeExported
 *   - relocated linked objects like PDF files
 *   - adds linked objects to $filesToBeExported
 *
 * @param string  &$HTMLcode                The html code to be altered.
 * @param int     $labNumber                The number of the lab to be exported.
 * @param array   &$IDmappingArray          The array containing the mappingbetween the element IDs.
 * @param array   &$ImagefilesToBeExported  Collects all image files  to be exported.
 * @param array   &$OtherfilesToBeExported  Collects all additional files  to be exported.
 * @return
 */
function processContent( &$HTMLcode, $labNumber, &$IDmappingArray, $doCopy=true, $isImport=false ){
	reMapIDs($HTMLcode, $labNumber, $IDmappingArray); // Replace the linked internal elements in $HTMLcode identified by address= or #
  if ( !$isImport ){ // Export
    processLocalPix($HTMLcode, $doCopy);
    processLocalFiles($HTMLcode, $doCopy);
  } else { // Import
    importPix($HTMLcode);
    importFiles($HTMLcode);
  }
}

////////////////////////////// called only internally /////////////////////////////////////////
/**
 * Remaps the relative internal links in $HTMLcode according to the $IDmappingArray.
 * The elements are identified by address= or # (anchor in page).
 *
 * @param string $HTMLcode        The HTML code. It will be altered during this processing.
 * @param int $labNumber          The number of the lab. The match is against IDs starting with l$labNumber.
 * @param array &$IDmappingArray  The array containing the mapping between the element IDs.
 *                                NOTE: The l? element must be included! (necessary at import).
 * @return
 */
  /** helper functions */
  /**
   * Maps the element to its mapping element or returns the emlement if no maping exists.
   * @param string $oldID  Element ID to be mapped.
   * @return string        The nes ID of the given element.
   */
  function mapIt( $oldID ){
    return array_key_exists( $oldID, $GLOBALS['IDmappingArray'] ) ? $GLOBALS['IDmappingArray'][$oldID] : $oldID;
  }
  /** end of functions */
function reMapIDs( &$HTMLcode, $labNumber, &$IDmappingArray){
  $GLOBALS['IDmappingArray'] = $IDmappingArray; // we need it in the call back (not clean implementation just work around).
	$HTMLcode= preg_replace_callback(  '#(?=address=|\#)'. // Assertion "must be preceded by" (link or anchor usage...
                                     '(l'.$labNumber.')'.              // Just replace things for the current lab
                                     '((?:\.[m, i, p, c, C]\pN)*)'.
                                     '#', // '{address=l, #l}(?P<data>(\.\p{LN})+)'
                                     create_function(
                                      '$matches',
                                      /**
                                       * The callback for preg_replace_callback.
                                       * It splits the elements and does the mapping for each element.
                                       *
                                       * @param array $matches  The matching array by evaluating the preg.
                                       * @return string         The replacement for the text.
                                       */
                                      '
                                        $parts = explode( ".", $matches[0] );
                                        foreach ( $parts as $key=>$value )
                                          $parts[ $key ] = mapIt( $value );
                                        return implode ( ".", $parts );
                                      '
                                     ),
                                     $HTMLcode
                                     );
}

/**
 * Handlers for images and a href linked LOCAL files.
 */
  /** helper functions */
  /**
   * Returns true if the url of the element is local.
   * @param string $fileWithPath  url.
   * @return bool
   */
  function isLocal( $fileWithPath ){
    return (substr( $fileWithPath, 0, 4 ) != 'http');
  }
  
  /** For Export:
   * If the file passed by $localFileWithPath is recognized as local and 
   * $doCopy is set (not the case for the HTML preview), then the file
   * is copied to $destDir.
   *
   * The new valid link to the file is returned.
   * (So if nothing is done the original link is returned.)
   *
   * @param string $localFileWithPath The url of the lnked file.
   * @param string $destDir           The directory where the file should be relocated if possible.
   * @param bool   $doCopy            Only if set copy action is done. Otherwhise just relocation (false for preview.html).
   * @return string                   New url of file.
   */
  function handleLocalFile( $localFileWithPath, $destDir, $doCopy ){
    global $cfg, $pge, $externallyLinkedElements;
   
    // maybe the file is fully linked but on the local server.
    // The following assumes we run just on the hosts root.
    $localFileWithPath = preg_replace_callback(  '#(http(s?)://)?'.$_SERVER['SERVER_NAME'].'#', 
                                                 create_function(
                                                                '$matches',
                                                                'return "..";'
                                                               ),
                                                 $localFileWithPath );

    // Files linked "/" should be "../"
    if ( $localFileWithPath[0] == '/' ) $localFileWithPath = '..'.$localFileWithPath;
    
    // not local: do nothing
    if ( !isLocal( $localFileWithPath ) ){
      if ($doCopy){
        $pge->put( 'non-local: |'.$localFileWithPath.'|<br>'."\r\n" );
        array_push( $externallyLinkedElements, $localFileWithPath );
      }
      return $localFileWithPath;
    }
    //break else:
    
    $fileName = end(explode('/', $localFileWithPath));
    if ($doCopy){
      // Here the move to server should happen...
      copy( $localFileWithPath, $cfg->get('exportImportDir').$GLOBALS['exportUID'].'/'.$destDir.$fileName )
      or trigger_error("Can't copy file ".$localFileWithPath, E_USER_WARNING);
      $pge->put(  '<div class="labsys_mop_elements_menu_s">'.
                  $localFileWithPath.
                  ' <img src="../syspix/button_export2disk_30x12.gif" width="30" height="12" border="0" alt="export" title="export">'.
                 "</div>\r\n" );
    }
    return $destDir.$fileName;
  }
  
  /** For Import
   * The file passed by $file2importWithPath
   * is copied to $destDir
   *
   * The new valid link to the file is returned.
   * (So if nothing is done the original link is returned.)
   *
   * @param string $file2importWithPath   File to be imported inclusive full path to the backupped file
   * @param string $destDir               The directory where the file should be relocated relative to the script without UID subdir.
   * @return string                       New url of file.
   */
  function importFile( $file2importWithPath, $destDir ){
    global $cfg, $pge;
    
    $destDir .= $GLOBALS['exportUID'].'/'; // add subdirectory
    $fileName = end(explode('/', $file2importWithPath));
    if ( !file_exists($destDir)) mkdir($destDir, 0755, true);
    
    copy( $file2importWithPath, $destDir.'/'.$fileName ) or trigger_error("Can't copy file ".$file2importWithPath, E_USER_ERROR);
    $pge->put(  '<div class="labsys_mop_elements_menu_s">'.
                ' <img src="../syspix/button_importFromDisk_30x12.gif" width="30" height="12" border="0" alt="import" title="import"> '.
                $fileName.
                "</div>\r\n" );
  
    return $destDir.$fileName;
  }
  /** end of functions */
/** EXPORT
 * Searches $HTMLcode for local images and if $doCopy is set copies them to the server/ directory.
 *
 * @param string &$HTMLcode          The html code to be altered.
 * @param bool   $doCopy             Only if set copy action is done. Otherwhise just relocation (false for preview.html).
 * @return                           Directly alters $HTMLcode.
 */
function processLocalPix( &$HTMLcode, $doCopy=true ){
	$HTMLcode = preg_replace_callback(   '#(?=<img )(.*? src=[\',"])(?!../syspix/|/syspix/)(\S+)([\',"])#',
                                       create_function(
                                        '$matches',
                                        'return $matches[1].handleLocalFile( $matches[2], \'images/\', '.intval($doCopy).' ).$matches[3];'
                                       ),
                                       $HTMLcode
                                       );
}
/** IMPORT
 * Searches $HTMLcode for images and if copies them to the $cfg->get("importPictureDir") directory.
 *
 * @param string &$HTMLcode          The html code to be altered.
 * @return                           Directly alters $HTMLcode.
 */
function importPix( &$HTMLcode ){
	$HTMLcode = preg_replace_callback(   '#(?=<img )(.*? src=[\',"])(images/\S+)([\',"])#',
                                       create_function(
                                        '$matches',
                                        'return $matches[1].importFile( $matches[2], $cfg->get("importPictureDir") ).$matches[3];'
                                       ),
                                       $HTMLcode
                                       );
}

/** EXPORT
 * Searches $HTMLcode for locally referenced files and if $doCopy is set copies them to the server/ directory.
 *
 * @param string &$HTMLcode          The html code to be altered.
 * @param bool   $doCopy             Only if set copy action is done. Otherwhise just relocation (false for preview.html).
 * @return                           Directly alters $HTMLcode.
 */
function processLocalFiles( &$HTMLcode, $doCopy=true ){
	$HTMLcode = preg_replace_callback(   '#(?=<a )(.*? href=[\',"])(?!../pages/|/pages/|\#)(\S+)([\',"])#',
                                       create_function(
                                        '$matches',
                                        'return $matches[1].handleLocalFile( $matches[2], \'data/\', '.intval($doCopy).' ).$matches[3];'
                                       ),
                                       $HTMLcode
                                       );
}
/** IMPORT
 * Searches $HTMLcode for referenced files and copies them to the $cfg->get("importFilesDir") directory.
 *
 * @param string &$HTMLcode          The html code to be altered.
 * @return                           Directly alters $HTMLcode.
 */
function importFiles( &$HTMLcode ){
  global $cfg;
	$HTMLcode = preg_replace_callback(   '#(?=<a )(.*? href=[\',"])(data/\S+)([\',"])#',
                                       create_function(
                                        '$matches',
                                        'return $matches[1].importFile( $matches[2], $cfg->get("importFilesDir") ).$matches[3];'
                                       ),
                                       $HTMLcode
                                       );
}

/**
 * Reindexes a collection's $content.
 *
 * @param string &$HTMLcode          The html code to be altered.
 * @param array &$IDmappingArray     The array containing the mapping between the element IDs.
 * @return
 */
function reindexCollectionElements( &$content, &$IDmappingArray ){
	$contents = explode(' ', $content);
	$content = '';
	foreach ( $contents as $element )
	  if (isset($IDmappingArray[$element])) // as there might have been non existing elements not for all values a mapping must exist!
	    $content .= ' '.$IDmappingArray[$element];
	$content = substr( $content, 1); // cut first space
}

/**
 * Called by createIdMapping as helper to remove stared values as they describe not existing elements
 *
 * @param sting $value   The value to be checked.
 * @return boolean       Is the value not starred?
 */
function notStared( $value ){
	return $value[0] != '*';
}

/**
 * Called in XIport's import and export to do the processing on the elements using the $mappingArray
 *
 * @param object $element       The element to be handled
 * @param object $mappingArray  The mapping array containing the replacement mapping for the element indices
 * @param int $labnumber        The number of the lab currently to be processed (for search and replace this is used as start marker for internal links)
 * @param bool $isImport=false  If set the import functions are called (for copying)
 * @return -
 */
function processElement( &$element, &$mappingArray, $labNumber, $isImport=false ){
  global $usr;
  switch ( $element->elementId ){
/*******************************************************
ToDo: the elements are loaded now. The data is in the respective fields.
Do all necessary parsings and store the files (txt+pictures+local_material)!
Fill $exportContent and write it at the bottom once!
*******************************************************/
    case 'l':
      // replace the elements according to the renaming:
        $nextElement->prelabCollectionIdx = substr( $mappingArray[ 'C'.$nextElement->preLab->idx ], 1 );
        $nextElement->labCollectionIdx    = substr( $mappingArray[ 'C'.$nextElement->lab->idx ], 1);
    // add export information to history
        $nextElement->history = retIfTrue( $isImport, 'Imported ', 'Exported ' ).' by '.$usr->foreName.' '.$usr->surName.' ('.$usr->userName.') from '.$_SERVER['SERVER_NAME'].' at '.date('r')."\r\n".$nextElement->history;
        processContent( $labToImport->title,  $labNumber, $mappingArray, true, $isImport );
        processContent( $labToImport->authors,$labNumber, $mappingArray, true, $isImport );
        processContent( $labToImport->comment,$labNumber, $mappingArray, true, $isImport );
      break;
    case 'C':
    case 'c':
      reindexCollectionElements( $nextElement->contents, $mappingArray );
      break;
    case 'p':
      processContent( $nextElement->title,    $labNumber, $mappingArray, true, $isImport );
      processContent( $nextElement->contents, $labNumber, $mappingArray, true, $isImport );
      break;
    case 'm':
      processContent( $nextElement->question, $labNumber, $mappingArray, true, $isImport );
      processContent( $nextElement->answerExplanation, $labNumber, $mappingArray, true, $isImport );
      for ($i=0; $i < $nextElement->answerCount; $i++)
        processContent( $nextElement->answerArray[$i], $labNumber, $mappingArray, true, $isImport );
      break;
    case 'i':
      processContent( $nextElement->question,       $labNumber, $mappingArray, true, $isImport );
      processContent( $nextElement->exampleSolution,$labNumber, $mappingArray, true, $isImport );
      break;
    default:
      $pge->put( 'ELEMENT NOT EXPORTED! '.$value );
      break;
  }
}

/**
 * getLabsFromDirectory( $cfg->get('exportImportDir') );
 *
 * 1) Reads all l0000001.txt files in all subdirectories of $directory.
 * 2) Transforms them into an l-Element.
 * 3) Puts them into an array
 * 4) Returns the Array
 *
 * ! sets the base64_encode( directory ) as ID for importable directories !
 *
 * @param sting $directory  The directory to be searched.
 * @return array            The array containing the l-Elements as values.
 */
function getLabsFromDirectory( $directory ){
  $retArray = array();
  if (!is_dir( $directory )) return $retArray;
	$subDirs = scandir( $directory );
  for( $i=0; $i<sizeof( $subDirs ); $i++){
    if ( $subDirs[$i] == '.' || $subDirs[$i] == '..' ) continue; // skip those
    $file_name = $directory.$subDirs[$i].'/l0000001.txt';
    if( file_exists($file_name) ){
      $myLelement = new LlElement( 0, 0, '', '', '', 1, 1, '', false, false, false, false, '' );
      $myLelement->initFromSerialized( file_get_contents( $file_name ) );
      $myLelement->idx = base64_encode( $subDirs[$i] ); // set the base64_encode( directory ) as ID for importable directories
      // the bas64 encoding is needed as the form.input in HTML replaces . by _ and so the name of the directory gets disturbed :(
      array_push( $retArray, $myLelement );
    }
  }
  return $retArray;
}

?>