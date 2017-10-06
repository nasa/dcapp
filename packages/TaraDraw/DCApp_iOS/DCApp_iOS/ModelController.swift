//
//  ModelController.swift
//  DCApp_iOS
//
//  Created by Shores, David L. (JSC-ER711) on 6/29/17.
//  Copyright © 2017 Shores, David L. (JSC-ER711). All rights reserved.
//

import UIKit


struct WindowData {
    
    init( widthA : Int16, heightA : Int16 )
    {
        width = widthA
        height = heightA
        needsUpdate = true
    }
    
    var needsUpdate : Bool  = true
    var width       : Int16 = 0
    var height      : Int16 = 0
}

protocol ModelControllerDelegate: class {
    func newViewAvailable( indexA : Int16 )
}

class ModelController: NSObject
{
    weak var delegate : ModelControllerDelegate?
    
    var displayPageID   = [Int16:WindowData]()

    override init() {
        super.init()
    }
    
    func getWindowData( indexA : Int16 ) -> WindowData!
    {
        if indexA >= 0 && indexA < Int16(displayPageID.count)
        {
            return displayPageID[indexA]
        }
        
        return nil
    }
    
    func markAllWindowsForRedraw()
    {
        for var window in displayPageID
        {
            window.value.needsUpdate = true
        }
    }
    
    @objc
    func addDisplayPage( widthA : Int16, heightA : Int16) -> Int16
    {
        let idL = Int16(displayPageID.count)
        displayPageID[idL]  = WindowData( widthA: widthA, heightA: heightA )
        
        delegate?.newViewAvailable(indexA: idL)
        
        return idL
    }
    
    @objc
    func setNeedsDisplay( indexA : Int16 )
    {
        if indexA >= 0 && indexA < Int16(displayPageID.count)
        {
            displayPageID[indexA]?.needsUpdate = true
        }
	}
	
	@objc
	func doesNeedDisplayNoClear( indexA : Int16 ) -> Bool
	{
		if indexA >= 0 || indexA < Int16(displayPageID.count)
		{
			return (displayPageID[indexA]?.needsUpdate)!
		}
		
		return false
	}
	
    @objc
    func doesNeedDisplay( indexA : Int16 ) -> Bool
    {
        if indexA >= 0 || indexA < Int16(displayPageID.count)
        {
            let valL = (displayPageID[indexA]?.needsUpdate)!
            
            displayPageID[indexA]?.needsUpdate = false
            
            return valL
        }

        return false
    }
}

